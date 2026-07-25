# Collector on Kubernetes

Issue #030. Runs the same image built for [Docker](docker.md)
(`docker/collector.Dockerfile`) as a self-healing `Deployment`, and proves it
with a live pod-delete demo: kill the pod, watch Kubernetes replace it,
watch the GUI recover without a restart.

The GUI is still not containerized or deployed to the cluster — same
reasoning as the Docker step. It's a native desktop client; here it connects
to the collector's Service instead of a locally-published container port.

## Manifests (`k8s/`)

- **`namespace.yaml`** — the `sentinelforge` namespace everything else lives in.
- **`configmap.yaml`** — two ConfigMaps, `collector-rules` and
  `collector-sigma-rules`, one key per file in `rules/` and `sigma-rules/`
  respectively (generated with `kubectl create configmap --from-file=... --dry-run=client -o yaml`,
  then hand-edited for namespace/labels). **This is the point of doing rules
  as a ConfigMap rather than baking them into the image**: editing a rule is
  `kubectl apply -f k8s/configmap.yaml` + a pod restart to pick up the new
  mount content, not a Docker rebuild. Regenerate after editing rule files:
  ```
  kubectl create configmap collector-rules --from-file=rules/ -n sentinelforge --dry-run=client -o yaml > /tmp/rules-cm.yaml
  kubectl create configmap collector-sigma-rules --from-file=sigma-rules/ -n sentinelforge --dry-run=client -o yaml > /tmp/sigma-cm.yaml
  cat /tmp/rules-cm.yaml /tmp/sigma-cm.yaml > k8s/configmap.yaml
  ```
  (manually re-add the `---` separator and confirm `metadata.name`/`namespace` survived the concat).
- **`deployment.yaml`** — `replicas: 1`, image `sentinelforge-collector:latest`
  with `imagePullPolicy: Never` (the image is expected to already be present
  in the cluster's local image store — see "Image" below, never pulled from
  a registry). The two ConfigMaps are mounted at `/src/rules` and
  `/src/sigma-rules`, the same paths the image's baked-in config
  (`docker/collector.config.json`, `api.bind_address: 0.0.0.0`) already
  points `rules_directory`/`sigma.rules_directory` at — nothing collector-side
  changed for Kubernetes, only how those directories get populated.
  `readinessProbe`/`livenessProbe` both hit `GET /health` on `8787`.
- **`service.yaml`** — `ClusterIP`, port `8787`. Selects `app: collector`, so
  it always routes to whatever pod currently matches that label — this is
  the piece that makes the self-healing demo actually demonstrate anything
  (see "Why NodePort didn't work" below for why it's `ClusterIP`, not
  `NodePort`, despite the port also being declared free in an earlier draft).

## Image

Docker Desktop's Kubernetes shares the same image store as `docker compose`/
`docker build` — an image built with `docker compose build` (issue #029) or
`docker build -f docker/collector.Dockerfile .` is already visible to the
cluster with no separate load step. This is specific to Docker Desktop; on
`kind` or `k3d` (neither is set up on this machine — checked `kind get
clusters` returns none, `k3d` isn't installed) the equivalent step would be
`kind load docker-image sentinelforge-collector:latest` or `k3d image import
sentinelforge-collector:latest`, and `deployment.yaml`'s `imagePullPolicy:
Never` would still be correct either way — this image is never meant to come
from a registry.

## Deploying

```
kubectl apply -f k8s/namespace.yaml -f k8s/configmap.yaml -f k8s/deployment.yaml -f k8s/service.yaml
kubectl -n sentinelforge get pods -w
```

Wait for `1/1 Running`. Confirm the config actually landed and rules loaded:

```
kubectl -n sentinelforge logs deploy/collector --tail 20
```

should show `ApiServer` listening on `0.0.0.0:8787` and `RuleLoader`/
`SigmaLoader` accepting the same rule counts as a local run.

## Reaching the API from the host — and why NodePort didn't work here

The manifest was originally written with `type: NodePort` (`nodePort: 30787`),
matching the more common local-cluster pattern. On this machine's Docker
Desktop Kubernetes it doesn't work: the Service's endpoint was correct
(`kubectl -n sentinelforge get endpoints collector` showed the pod IP), but
neither `curl http://localhost:30787/health` nor `curl
http://<node-internal-ip>:30787/health` connected — `localhost` was refused
outright (nothing bound there) and the node's internal container IP timed
out (not routable from the Windows host at all). Docker Desktop's Kubernetes
does not auto-publish the NodePort range to the Windows host the way
`docker run -p` publishes container ports. Rather than fight that, the
Service is `ClusterIP` and reachability from the host goes through
`kubectl port-forward`, which does work reliably.

**One real wrinkle**: `kubectl port-forward svc/collector 8787:8787` looks
like it forwards through the Service, but it doesn't — it resolves the
Service to one specific pod once, at the moment it starts, and tunnels
straight to that pod's sandbox. When that pod is deleted, the forward dies
outright (`error: lost connection to pod`) instead of following the Service
to the replacement. That's a `kubectl port-forward` limitation, not a
Service problem — the Service itself (`kubectl -n sentinelforge get
endpoints collector`) updates its endpoint immediately when the new pod
turns Ready. For a demo where the pod is deliberately killed, `port-forward`
needs to be run in a restart loop so the host-side tunnel survives the pod
being replaced underneath it:

```
while true; do
  kubectl -n sentinelforge port-forward svc/collector 8787:8787
  sleep 0.5
done
```

```
curl http://localhost:8787/health
```

## Connecting the GUI

```
gui/build/bin/Debug/sentinelforge_desktop.exe
```

Same as the Docker case: `CollectorTelemetrySource`'s default base URL is
`http://127.0.0.1:8787`, which is exactly what the port-forward loop above
publishes, so no flags are needed. Make sure nothing else is already bound
to host `8787` first (e.g. a `docker compose up` collector from issue #029)
— `docker compose stop` frees it.

## The self-healing demo

With the port-forward loop and the GUI both running against it:

```
kubectl -n sentinelforge get pods
# NAME                         READY   STATUS    RESTARTS   AGE
# collector-698d87f6d5-x5rrk   1/1     Running   0          50s

kubectl -n sentinelforge delete pod collector-698d87f6d5-x5rrk
```

What happens, in order, timed on this machine:

1. **Immediately** — the ReplicaSet notices the pod is gone and schedules a
   replacement (`kubectl -n sentinelforge get pods` shows a new pod name,
   `0/1`, within ~1-2s of the delete).
2. **~0.5-1s** — the port-forward loop's `kubectl port-forward` process dies
   with the pod (it was tunneled directly to that pod's now-gone sandbox)
   and the loop's `sleep 0.5` + restart re-issues `port-forward svc/collector`,
   which resolves to whatever pod the Service currently points at.
3. **~5-8s** — the new pod passes its readiness probe (`initialDelaySeconds:
   2`, `periodSeconds: 3`) and the Service's endpoint list updates to point
   at it; `curl http://localhost:8787/health` starts succeeding again.
4. **The GUI** never restarts or crashes through any of this — same OS
   process the whole time (confirmed by PID). `CollectorTelemetrySource`
   polls every 500ms; a handful of failed polls during the gap above trip
   its `ConnectionState::Connected -> Reconnecting` transition
   (`kFailedThreshold = 3` consecutive failed ticks before it would escalate
   to `Failed`, which — on this machine — the recovery window was fast
   enough to not even reach), and the next successful poll once the new pod
   is Ready flips it straight back to `Connected`. This is the same
   reconnect state machine documented in `CollectorTelemetrySource.cpp` — it
   was not written for this demo, the pod-delete demo just exercises it.

Verify the new pod is what actually served the recovered request:

```
kubectl -n sentinelforge get pods
kubectl -n sentinelforge logs deploy/collector --tail 5
```

the pod name and `uptime_seconds` in `/health` (freshly near-zero) will have
changed from before the delete.

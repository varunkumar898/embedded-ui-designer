# Production Deployment Runbook

## Overview

This runbook outlines the operational procedures for deploying, maintaining, and recovering the **Embedded UI Designer** platform in staging and production environments.

---

## 1. Pre-Deployment Checklist

- [ ] All 102 acceptance tests pass cleanly (`ctest --test-dir build_test`, `node integration_runner.js`).
- [ ] Static Application Security Testing (SAST) passes with 0 critical/high findings.
- [ ] Dependencies scanned against OWASP database (`dependency-check-report.json`).
- [ ] Target firmware signed with production private key (RSA-2048 or ECC P-256).
- [ ] Differential patch verified against current production base version.
- [ ] Staging health probes return HTTP 200 `{"status": "ok"}`.

---

## 2. Deployment Strategies

### Blue-Green Deployment Procedure
1. **Deploy to Blue (Staging)**:
   ```bash
   kubectl set image deployment/embedded-ui-staging firmware=registry.company.com/embedded-ui:v1.2.0-staging
   kubectl rollout status deployment/embedded-ui-staging --timeout=5m
   ```
2. **Execute Smoke Tests on Blue**:
   ```bash
   ./verification-tests.sh http://staging.device.internal:8080
   ```
3. **Manual Approval Gate**:
   Release team confirms QA sign-off.
4. **Switch Traffic to Green (Production)**:
   ```bash
   kubectl set image deployment/embedded-ui-prod firmware=registry.company.com/embedded-ui:v1.2.0
   kubectl rollout status deployment/embedded-ui-prod --timeout=5m
   ```
5. **Run Post-Deployment Health Probes**:
   ```bash
   ./health-check.sh http://production.device.internal:8080
   ```

---

## 3. Automated Rollback Trigger

If the health check fails 3 consecutive times within 300 seconds, the CI/CD pipeline triggers an immediate automatic rollback:

```bash
kubectl rollout undo deployment/embedded-ui-prod
```

For bare-metal and physical embedded units:
```bash
# Roll back firmware to previous known-good slot (Dual-bank flash)
openocd -f interface/stlink.cfg -c "init; reset init; stm32f0x rollback; exit"
```

---

## 4. Post-Deployment Verification

* Check Prometheus dashboard for latency metrics (`http_request_duration_ms`).
* Confirm P99 latency remains under 100ms.
* Check audit logs in Elasticsearch for authentication failures or unhandled errors.
* Validate memory watermark on physical nodes remains below 85% maximum heap.

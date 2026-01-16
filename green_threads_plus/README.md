# GreenThreads++: Research-Grade User-Space Threading Platform

GreenThreads++ has been evolved into a comparative analysis platform for researching user-space (Green) vs kernel-space (POSIX) threading models. It features a sophisticated "Dual Runtime" engine capable of executing workloads simultaneously on both models, monitored by the **Adaptive User-Space Runtime Analyzer (AURA)**.

## 🔬 Research Capability: AURA
The **Adaptive User-Space Runtime Analyzer (AURA)** is an embedded subsystem that monitors the runtime health of green threads.
- **Starvation Detection**: Alerts when tickets/priorities prevent runs > 500ms.
- **Fairness Analysis**: Calculates Stride deviation.
- **Auto-Remediation**: Can dynamically boost tickets to resolve starvation.

## ⚔️ Dual-Runtime Engine
Run workloads in three modes:
1.  **Green Only**: Pure user-space cooperative/preemptive multitasking.
2.  **Pthread Only**: Kernel-managed 1:1 threading.
3.  **Dual Mode**: Simultaneous execution for side-by-side performance profiling.

## 📊 Real-Time Observability Stack
- **Dashboard**: React + Tailwind + Recharts (Updates every 100ms via WebSockets).
- **Backend Relay**: Node.js WebSocket Bridge.
- **Metrics Warehouse**: Prometheus (1s resolution) + Grafana.

---

## 🚀 Quick Start (Docker)

The easiest way to run the full stack is with Docker Compose.

```bash
cd docker
docker-compose up --build
```

### Access Points
- **Dashboard**: [http://localhost:80](http://localhost:80) (React UI)
  - Use the "Spawn" button to create workloads.
  - Experiment with "Time Slice" and "Tickets" sliders.
  - "Run Experiment (Ticket Sweep)" will auto-run a stress test.
- **Prometheus**: [http://localhost:9090](http://localhost:9090)
- **Grafana**: [http://localhost:3000](http://localhost:3000) (Login: admin/admin)
  - Go to Dashboards -> Import -> Upload JSON (or use pre-provisioned).

### Architecture
1.  **C Runtime (`src/`)**: Dual-mode (Green + Pthread) engine with cooperative/preemptive (simulated) scheduling.
2.  **Node.js Relay (`server/`)**: Bridges WebSocket (UI) to TCP (C App).
3.  **Frontend (`frontend/`)**: React + Recharts + Tailwind.
4.  **Observability**: Metrics pushed to Prometheus, analyzed by "Aura" (Anomaly Detection) in real-time.

### Deployment & Testing
- **Experiment Mode**: Click "Run Experiment (Ticket Sweep)" in the UI to automatically vary thread priority and observe context switch rates.
- **Logs**: Check `docker-compose logs -f app` to see AURA anomaly alerts (e.g. Starvation Detected).

### 3. Run a Comparative Workload
From the Dashboard or CLI, spawn a matrix multiplication task:
```bash
# Example Command via CLI (future extension) or Dashboard Button
spawn:matrix:50
```
Watch the "Green" vs "Pthread" counters race in real-time.

---

## 📂 Architecture

```
[ Frontend (React) ] <==> [ WebSocket Relay ] <==> [ C Runtime (TCP Output) ]
                                                         |
                                         +---------------+---------------+
                                         |                               |
                                  [ Green Scheduler ]            [ Pthread Pool ]
                                         |                               |
                                  [ AURA Analyzer ]              [ OS Kernel ]
```

## 🛠️ Development
- **New Workloads**: Add to `src/workloads.c`.
- **AURA Logic**: Modify `src/aura.c`.
- **Metrics**: Define new counters in `src/metrics.h`.

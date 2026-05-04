# CertOS 🚀
![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)

CertOS is a specialized, bootable Linux distribution designed for HPC (High Performance Computing) and private cloud environments. It transforms standard hardware into a managed cluster node with job isolation and automated updates.

## 💿 Quick Start
1. **Download**: Get the latest ISO from the [Releases](https://github.com/moinmoin-64/certos/releases) page.
2. **Install**: Boot from the ISO and follow the TUI Wizard to configure your node as a Master or Agent.
3. **Connect**: Access the Web Gateway at `http://<master-ip>:8080`.

## Features

- **Distributed Architecture**: 3-plane design (Control, Compute, Access).
- **gRPC Protocol**: High-throughput communication between Master and Node Agents.
- **Fair-Share Scheduler**: Priority-aware scheduling engine with dynamic usage decay.
- **Role-Based Access Control (RBAC)**: Secure internal node communication and external user JWT authentication.
- **System Telemetry**: Real-time Node CPU, RAM, and Job monitoring.
- **Web Dashboard**: A beautiful, modern GUI to track cluster health, heatmap, and queue.

## System Components

1. **Master Node (`certosc-master`)**: The Control Plane. Orchestrates jobs, maintains the node registry, runs the scheduler engine, and persists state in SQLite.
2. **Node Agent (`certosc-agent`)**: The Compute Plane. Deployed on worker servers. Registers with the master, streams hardware metrics, and executes assigned tasks in isolated environments.
3. **Gateway (`certosc-gateway`)**: The Access Layer. A Boost.Beast HTTP/WebSocket bridge connecting external REST/Web clients to the internal gRPC services.
4. **Dashboard (`web/`)**: The presentation layer.

## Requirements

- **OS**: Linux (or Windows WSL2 Ubuntu)
- **Compiler**: GCC 9+ or Clang 10+ (C++17 Support)
- **CMake**: >= 3.20
- **vcpkg**: Microsoft's C++ package manager

### Dependencies (Managed via vcpkg)
- gRPC & Protobuf
- Boost (Beast, Asio, JSON)
- SQLite3
- spdlog & fmt
- yaml-cpp
- jwt-cpp
- OpenSSL

## Build Instructions

1. Ensure `VCPKG_ROOT` is exported in your environment:
   ```bash
   export VCPKG_ROOT=~/vcpkg
   ```
2. Run the build script (this will configure CMake, install dependencies, and build all targets):
   ```bash
   ./scripts/build.sh
   ```

## Running a Local Cluster

To quickly test CertOS locally, you can use the automated cluster runner which spins up a Master, a Gateway, and 2 mock Node Agents:

```bash
./scripts/run_cluster.sh
```

Then, open your browser and navigate to: `http://localhost:8080/index.html` (if serving statically) or simply open `web/index.html` directly in your browser.

## Configuration

Settings are controlled via YAML files located in `config/`:
- `master.yaml` - Scheduler intervals, DB path, binding addresses.
- `agent.yaml` - Master location, hardware overrides.
- `gateway.yaml` - REST HTTP ports, CORS.

## Project Phase

We are currently completing **Phase 4: Production Hardening**.
- ✅ Foundation & Scaffolding
- ✅ REST API & Web Dashboard
- ✅ Advanced Fair-Share Scheduler
- ✅ Unit Tests & Scripts
- ⏳ Full Integration Testing

---
*Developed by the CertOS Team.*

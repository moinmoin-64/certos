#!/bin/bash
set -e

# Run this script to start a local 3-node cluster for testing
# It starts the Master, the Gateway, and 2 Agents in the background.

cd "$(dirname "$0")/.."

BUILD_DIR="build"

if [ ! -f "$BUILD_DIR/master/certosc-master" ]; then
    echo "Executables not found. Please run ./scripts/build.sh first."
    exit 1
fi

echo "Creating log and config directories..."
mkdir -p logs config /tmp/certosc-jobs

echo "Killing any existing instances..."
pkill -f certosc-master || true
pkill -f certosc-agent || true
pkill -f certosc-gateway || true
sleep 1

echo "Starting certosc-master..."
./$BUILD_DIR/master/certosc-master config/master.yaml > logs/master_stdout.log 2>&1 &
MASTER_PID=$!
sleep 1

echo "Starting certosc-gateway..."
./$BUILD_DIR/gateway/certosc-gateway config/gateway.yaml > logs/gateway_stdout.log 2>&1 &
GATEWAY_PID=$!
sleep 1

echo "Starting certosc-agent (Node 1)..."
./$BUILD_DIR/agent/certosc-agent config/agent.yaml > logs/agent1_stdout.log 2>&1 &
AGENT1_PID=$!

echo "Starting certosc-agent (Node 2)..."
./$BUILD_DIR/agent/certosc-agent config/agent.yaml > logs/agent2_stdout.log 2>&1 &
AGENT2_PID=$!

echo "=========================================================="
echo " CertOS Local Cluster Running!"
echo "=========================================================="
echo " - Master PID:  $MASTER_PID"
echo " - Gateway PID: $GATEWAY_PID"
echo " - Agent 1 PID: $AGENT1_PID"
echo " - Agent 2 PID: $AGENT2_PID"
echo ""
echo " Dashboard available at: http://localhost:8080/index.html"
echo " (Assuming you serve the web/ directory statically, or open index.html directly)"
echo " To stop the cluster, run: pkill -f certosc-"
echo "=========================================================="

# Keep script running to allow easy Ctrl+C shutdown
trap "echo 'Shutting down cluster...'; kill $MASTER_PID $GATEWAY_PID $AGENT1_PID $AGENT2_PID; exit 0" SIGINT SIGTERM
wait

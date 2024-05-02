# Rollerball Chess Engine - IIT Delhi COL333

A high-performance C++ game-playing engine for **Rollerball Chess**, a chess variant with a rotating board geometry. Built for IIT Delhi's Artificial Intelligence course (COL333).

## Highlights

- **Custom evaluation function** over piece material, mobility, king safety, board control, pawn promotion distance, and piece synergy
- **Board transforms** (90Â° / 180Â° rotations) baked into move generation for the rollerball board topology
- **Search-driven play** with an atomic best-move interface designed for timed matches
- **WebSocket / UCI-WS bridge** so the engine can talk to a browser frontend
- Optional **Python bindings** path via `pybind11` (`make rollerball_py`)

## Architecture

```
src/
  board.cpp / board.hpp     # Board state, move gen, transforms
  engine.cpp / engine.hpp   # Search + evaluation
  server.cpp / server.hpp   # Match server
  uciws.cpp / uciws.hpp     # UCI-over-WebSocket protocol
  rollerball.cpp            # Entrypoint
```

## Build

```bash
# Requires: g++ with C++17, pthread
make rollerball
./bin/rollerball
```

For the Python-integrated build:

```bash
pip install pybind11
make rollerball_py
```

> Note: the original course scaffold shipped third-party headers under `include/` (Asio, etc.). Point `-Iinclude` at your local Asio install, or vendor Asio yourself.

## Design notes

The evaluator aggregates per-player stats (`EvalStats`) - kings/bishops/rooks/pawns, check status, legal-move counts, promotion proximity, board control, and synergy - then scores them with tunable weights. Attack maps (`positionsAttackedBy` / `positionUnderAttackBy`) support richer tactical features without rescanning the board every time.

## Course

**COL333 - Artificial Intelligence**, IIT Delhi  
Assignment: Rollerball Chess agent

## License

Coursework / educational use. Not affiliated with IIT Delhi officially.

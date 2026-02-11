**Distributed Logger**

High-performance structured event logging for distributed C/C++ systems with pluggable storage backends and code-generated APIs.

Distributed Logger is designed for systems where traditional logging becomes a performance bottleneck or lacks structured query capabilities. It provides a lightweight wire protocol, generated strongly-typed logging APIs, and scalable ingestion pipelines targeting databases such as MongoDB and ClickHouse.

**Why Distributed Logger Exists**

Modern distributed services generate large volumes of diagnostic and performance data. Traditional logging approaches often suffer from:

* High runtime overhead

* Poor structure and queryability

* Difficult correlation across machines and threads

* Limited support for high-performance C/C++ environments

* Tight coupling between logging format and storage backend

Distributed Logger addresses these challenges by treating logs as **structured events** transmitted efficiently to centralized storage.

**Key Features**
⚡ **High-Performance Event Logging**

* Minimal binary wire protocol

* Append-only ingestion pipeline

* Designed for high-throughput distributed systems

🧬 **Code-Generated Logging APIs**

* Users define events via simple C++ header declarations

* Generator produces strongly-typed client and server logic

* Eliminates runtime reflection and schema mismatches

🗄 **Pluggable Storage Backends**

Currently supported:

* MongoDB

* ClickHouse

Architecture supports adding new backends without modifying client code.

🔌 **Customizable Transport and Buffering**

* TCP and TLS support

* User-replaceable IO and buffer implementations

* Seastar compatible

🧵 **Scalable Ingestion Pipeline**

* Worker-based batching

**When To Use Distributed Logger**

Distributed Logger is particularly useful for:

* High-performance distributed services

* Systems requiring structured event analysis

- C/C++ infrastructure or backend services

* Performance monitoring and troubleshooting pipelines

* Systems where log queryability is critical

**When Not To Use It**

Distributed Logger is **not** intended to replace:

* General purpose logging frameworks (spdlog, log4j, etc.)

* Metrics/observability stacks like OpenTelemetry

* Real-time alerting systems

It is optimized for **structured event capture and post-analysis.**

**Architecture Overview**

Client Application -> Generated Event Encoder -> TCP / TLS Transport -> Listener -> Generated Decoder & Dispatch -> Batch Workers -> Storage Backend (MongoDB / ClickHouse)

**Design Principle**

Only the decoder layer interacts with wire format.
Storage layers operate on strongly typed event structures.

**Quick Start**
1. **Define Event API**

Example event definition:

`#pragma once`

`void LogEvent(uint64_t event0, uint64_t shard, std::string host);`

 `void LogEvent(uint64_t event1, uint64_t shard, std::string host, uint64_t timestamp);`

2. **Run Code Generator**

`python generator.py --input event_api.hh`


Generated components include:

* Client encoders

* Server decoders

* Storage adapters

3. **Start Server**

`go run server/main.go --config config.json`

5. **Log Events From Client**
`Logger<MyBuffer, MyIO> logger(io);
logger.logEvent(event0, shard, host);`

**Supported Client Languages**
**Language	Status**
C++	✅ Stable
Go	🚧 Planned
Python	💡 Planned

The wire protocol and generator architecture are language-agnostic and designed to support additional client implementations.

**Storage Model**

**MongoDB**

* Direct structured event insertion

* Flexible schema

**ClickHouse**

* Append-only ingestion table

* Optional materialized views for typed event tables

* Optimized for analytics workloads

**Customization**

Distributed Logger allows customization of:

* IO transport implementations

* Buffer management

* Storage backends

* Code generation extensions

See docs/customization.md for details.

**Protocol Overview**

Events are transmitted using a lightweight binary frame:

`[uint32 packet_length]
[uint64 event_id]
[payload...]`


Payload encoding is generated based on user-defined event signatures.

Full specification: docs/protocol.md

**Project Status**

Current focus:

* Stabilizing wire protocol

* Storage performance optimization

* Generator extensibility

**Contributing**

Contributions are welcome, including:

* Additional storage backends

* Client language generators

* Performance improvements

* Documentation enhancements

See CONTRIBUTING.md.

**License**

This project is licensed under the Apache License 2.0.

You are free to use, modify, and distribute this software, including in commercial products.

The license includes an explicit patent grant from contributors, which helps protect users and adopters of the project.

See the LICENSE file for full details.



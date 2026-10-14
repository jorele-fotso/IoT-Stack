# IoT-Stack – Vitals & Air Monitoring (Old People)

login:iotserver password:test..123

Server side of our school IoT project (LGK, BTS Communication Technologies).
An ESP32-S3 device monitors an elderly person's room; this repo receives, stores and displays its data with Docker.

## Data flow

```
ESP32-S3 + sensors ──Wi-Fi──> Mosquitto (MQTT) ──> Telegraf ──> InfluxDB ──> Grafana
```

| Service | Role | Status |
|---|---|---|
| InfluxDB | Stores the measurements | ✅ Running |
| Grafana | Dashboard and alerts | ✅ Running |
| Mosquitto | Receives the ESP32 messages (MQTT) | ⏳ To do |
| Telegraf | Writes MQTT messages into InfluxDB | ⏳ To do |

**Server:** unknown for now (tested on an Ubuntu Server VM).

## Quick start

```bash
git clone https://github.com/jorele-fotso/IoT-Stack.git && cd IoT-Stack
cp .env.example .env   # then set your own password and token
docker compose up -d
```

`.env` contains the secrets and is never pushed to GitHub.

| Service | URL |
|---|---|
| InfluxDB | `http://<server-ip>:8086` |
| Grafana | `http://<server-ip>:3000` (first login `admin` / `admin`) |

## Team

| Member | Part |
|---|---|
| Abdul | SCD41 – CO2, temperature, humidity |
| Daniel | SPS30 – fine particles, 3D enclosure |
| David | mmWave radar – presence, movement, falls |
| Jawad | RFID RC522 – medication, 3D enclosure |
| Noé | MQTT (Mosquitto) |
| Jorèle | Docker, InfluxDB, Grafana |

## Roadmap

- [x] InfluxDB + Grafana with Docker Compose
- [ ] Mosquitto + Telegraf
- [ ] Test with simulated data
- [ ] Grafana dashboard and alerts
- [ ] Connect the real ESP32-S3
- [ ] Deploy on the final server (unknown for now)

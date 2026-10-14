# IoT-Stack – Vitals & Air Monitoring (Old People)

Server side of our school IoT project (LGK, BTS Communication Technologies).
An ESP32-S3 device monitors an elderly person's room (air quality, presence, falls, medication).
This repository contains the **Docker setup** that receives, stores and displays that data.

## How the data flows

```
ESP32-S3 + sensors ──Wi-Fi──> MQTT broker ──> Telegraf ──> InfluxDB ──> Grafana
                              (Mosquitto)                  (storage)    (dashboard + alerts)
```

| Service | Role | Status |
|---|---|---|
| **InfluxDB 2** | Time-series database: stores every measurement with its time | ✅ Running |
| **Grafana** | Web dashboard: graphs and alerts | ✅ Running |
| **Mosquitto** | MQTT broker: receives the messages sent by the ESP32 | ⏳ Next step |
| **Telegraf** | Reads the MQTT messages and writes them into InfluxDB | ⏳ Next step |

## Requirements

- A Linux machine or VM (we use **Ubuntu Server**)
- **Docker** and the **Docker Compose** plugin
- Git

## Quick start

```bash
# 1. Get the code
git clone https://github.com/jorele-fotso/IoT-Stack.git
cd IoT-Stack

# 2. Create your own secrets file from the template
cp .env.example .env
nano .env        # set a real password and a long random token

# 3. Start all services in the background
docker compose up -d

# 4. Check that the containers are running
docker compose ps
```

> `.env` holds passwords and the InfluxDB token. It is listed in `.gitignore`,
> so it is **never** pushed to GitHub. Only `.env.example` (with fake values) is shared.

## Access

Replace `<server-ip>` with the IP address of the VM/server (`ip a` on the server).

| Service | URL | Login |
|---|---|---|
| InfluxDB | `http://<server-ip>:8086` | Username and password from `.env` |
| Grafana | `http://<server-ip>:3000` | `admin` / `admin` on first login (you are asked to change it) |

On the first start, InfluxDB is created automatically with the organisation,
bucket and token defined in `.env`.

## Configuration (`.env`)

| Variable | Meaning |
|---|---|
| `INFLUXDB_USERNAME` / `INFLUXDB_PASSWORD` | InfluxDB admin account |
| `INFLUXDB_ORG` | Organisation name (`lgk`) |
| `INFLUXDB_BUCKET` | Where sensor data is stored (`sensors`) |
| `INFLUXDB_TOKEN` | API key used by Telegraf and Grafana to access InfluxDB |

## Useful commands

```bash
docker compose ps              # list containers and their status
docker compose logs -f grafana # follow the logs of one service
docker compose down            # stop everything (data is kept in volumes)
git pull && docker compose up -d   # update to the latest version
```

## Data format (for sensor owners)

To be stored and displayed automatically, every sensor must publish to MQTT with:

- **Topic:** `elderly/<device_id>/<sensor>` – e.g. `elderly/room1/air`
- **Payload:** one flat JSON object with numeric values,
  e.g. `{"co2_ppm": 850, "temperature_c": 22.4, "humidity_pct": 45.1}`

Full details and questions for each member are in the **Data Format Request** document shared with the group.

## Team

| Member | Part |
|---|---|
| Abdul | SCD41 – CO2, temperature, humidity |
| Daniel | SPS30 – fine particles, 3D enclosure |
| David | mmWave radar – presence, movement, falls |
| Jawad | RFID RC522 – medication, 3D enclosure |
| Jorèle | Server: Docker, InfluxDB, Grafana |
| Noé | TBD |

## Roadmap

- [x] Ubuntu Server VM with Docker
- [x] InfluxDB + Grafana with Docker Compose, secrets in `.env`
- [ ] Add Mosquitto (MQTT) and Telegraf
- [ ] Test with simulated sensor data
- [ ] Build the Grafana dashboard and alerts
- [ ] Connect the real ESP32-S3
- [ ] Move to the final server (school server or cloud)

# IoT Stack

A minimal, self-hosted IoT monitoring stack built with Docker Compose. It stores sensor time-series data in **InfluxDB** and lets you explore it with dashboards in **Grafana**.

## How it works

The stack is made of two services defined in `docker-compose.yml`:

| Service    | Image                | Port   | Role                                                                 |
|------------|----------------------|--------|----------------------------------------------------------------------|
| `influxdb` | `influxdb:2`         | `8086` | Time-series database. Your sensors/devices write measurements to it. |
| `grafana`  | `grafana/grafana-oss`| `3000` | Visualization layer. Reads data from InfluxDB and renders dashboards. |

Data flow:

```
IoT devices / scripts  ──write──▶  InfluxDB (:8086)  ◀──query──  Grafana (:3000)  ──▶  your browser
```

- On first start, InfluxDB is automatically initialized (`DOCKER_INFLUXDB_INIT_MODE=setup`) with the admin user, organization, bucket and API token defined in your `.env` file.
- Grafana starts after InfluxDB (`depends_on`).
- Data is persisted in two named Docker volumes, `influxdb-data` and `grafana-data`, so it survives container restarts and re-creation.
- Both services use `restart: unless-stopped`.

## Prerequisites

- [Docker](https://docs.docker.com/get-docker/)
- [Docker Compose](https://docs.docker.com/compose/install/) v2 (included with Docker Desktop)

## Configuration

Configuration is read from a `.env` file. A template is provided in `.env.example`.

1. Copy the example file:

   ```bash
   cp .env.example .env
   ```

2. Edit `.env` and replace the placeholder values:

   | Variable            | Default   | Description                                           |
   |---------------------|-----------|-------------------------------------------------------|
   | `INFLUXDB_USERNAME` | `admin`   | InfluxDB admin username                               |
   | `INFLUXDB_PASSWORD` | `change-me` | InfluxDB admin password (**change it**)             |
   | `INFLUXDB_ORG`      | `lgk`     | InfluxDB organization name                            |
   | `INFLUXDB_BUCKET`   | `sensors` | Initial bucket where sensor data is stored            |
   | `INFLUXDB_TOKEN`    | `change-me` | Admin API token used by devices and Grafana (**change it**) |

   Use a strong password and a long random token, for example:

   ```bash
   openssl rand -hex 32
   ```

> The `.env` file contains secrets. Do not commit it to version control.

> The `INFLUXDB_*` init values are only applied the **first time** InfluxDB starts with an empty data volume. Changing them later has no effect unless you remove the volume (see below).

## Running the stack

Start both services in the background:

```bash
docker compose up -d
```

Check that they are running:

```bash
docker compose ps
```

Follow the logs:

```bash
docker compose logs -f
```

## Accessing the services

- **InfluxDB UI:** <http://localhost:8086> — log in with `INFLUXDB_USERNAME` / `INFLUXDB_PASSWORD`.
- **Grafana:** <http://localhost:3000> — default login is `admin` / `admin` (you will be asked to change the password).

### Connect Grafana to InfluxDB

In Grafana, go to **Connections → Data sources → Add data source → InfluxDB** and set:

- **Query language:** Flux
- **URL:** `http://influxdb:8086` (the service name resolves inside the Compose network)
- **Organization:** value of `INFLUXDB_ORG`
- **Token:** value of `INFLUXDB_TOKEN`
- **Default bucket:** value of `INFLUXDB_BUCKET`

Then click **Save & test**.

### Send data from a device

Write a data point with the InfluxDB HTTP API (replace the values with those from your `.env`):

```bash
curl --request POST \
  "http://localhost:8086/api/v2/write?org=lgk&bucket=sensors&precision=s" \
  --header "Authorization: Token <INFLUXDB_TOKEN>" \
  --data-raw "temperature,room=kitchen value=21.5"
```

## Stopping the stack

```bash
docker compose down
```

To also delete all stored data (InfluxDB and Grafana volumes) and start from scratch:

```bash
docker compose down -v
```

#!/usr/bin/env python3
"""Fake ESP32: publishes simulated sensor data to Mosquitto.

Lets us test Telegraf, InfluxDB and Grafana before the real hardware arrives.
Format: see the Data Format Request v2 (topics elderly/room1/<part>, JSON numbers).

Usage (from the repository root, on the server):
    sudo apt install python3-paho-mqtt      # once
    python3 simulator/simulate.py           # stop with Ctrl+C
    python3 simulator/simulate.py --host 192.168.188.154   # from another computer
"""
import argparse
import json
import random
import time
from pathlib import Path

import paho.mqtt.client as mqtt

ROOM = "room1"
ENV_FILE = Path(__file__).resolve().parent.parent / ".env"


def read_env(path):
    """Read KEY=VALUE lines from the .env file (MQTT user and password)."""
    values = {}
    if path.exists():
        for line in path.read_text().splitlines():
            line = line.strip()
            if line and not line.startswith("#") and "=" in line:
                key, value = line.split("=", 1)
                values[key.strip()] = value.strip().strip('"').strip("'")
    return values


def make_client():
    """Create an MQTT client (works with paho-mqtt 1.x and 2.x)."""
    try:
        return mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="simulator")
    except AttributeError:
        return mqtt.Client(client_id="simulator")


def clamp(value, low, high):
    return max(low, min(high, value))


def main():
    env = read_env(ENV_FILE)
    parser = argparse.ArgumentParser(description="Publish fake sensor data over MQTT")
    parser.add_argument("--host", default="localhost", help="MQTT broker address")
    parser.add_argument("--port", type=int, default=1883)
    parser.add_argument("--user", default=env.get("MQTT_DEVICE_USER", ""))
    parser.add_argument("--password", default=env.get("MQTT_DEVICE_PASSWORD", ""))
    parser.add_argument("--interval", type=int, default=10, help="seconds between air/particle/radar messages")
    args = parser.parse_args()

    client = make_client()
    if args.user:
        client.username_pw_set(args.user, args.password)
    client.connect(args.host, args.port)
    client.loop_start()
    print(f"Connected to {args.host}:{args.port} - publishing every {args.interval} s (Ctrl+C to stop)")

    def publish(part, payload):
        topic = f"elderly/{ROOM}/{part}"
        client.publish(topic, json.dumps(payload))
        print(topic, payload)

    co2, temp, hum, pm25 = 650.0, 21.5, 45.0, 6.0
    start = time.time()
    last_status = 0.0
    try:
        while True:
            # Air: slow random walk, CO2 sometimes climbs to test the alerts
            co2 = clamp(co2 + random.uniform(-30, 45), 420, 1800)
            temp = clamp(temp + random.uniform(-0.2, 0.2), 16, 30)
            hum = clamp(hum + random.uniform(-1, 1), 25, 75)
            publish("air", {"co2_ppm": round(co2), "temperature_c": round(temp, 1), "humidity_pct": round(hum, 1)})

            # Fine particles (µg/m³)
            pm25 = clamp(pm25 + random.uniform(-1, 1), 1, 60)
            publish("particles", {
                "pm1_ugm3": round(pm25 * 0.7, 1),
                "pm2_5_ugm3": round(pm25, 1),
                "pm4_ugm3": round(pm25 * 1.15, 1),
                "pm10_ugm3": round(pm25 * 1.3, 1),
            })

            # Radar: 0 none, 1 still, 2 active; a fall now and then (about 1 in 50)
            presence = 1 if random.random() < 0.9 else 0
            movement = random.choice([1, 1, 2]) if presence else 0
            fall = 1 if presence and random.random() < 0.02 else 0
            publish("radar", {"presence": presence, "movement": movement, "fall": fall})

            # Medication: a badge scan now and then (about 1 in 15)
            if random.random() < 0.07:
                badge, name = random.choice([("04A1B2C3", "Paracetamol"), ("04D5E6F7", "Vitamin D")])
                publish("medication", {"badge_uid": badge, "medication": name, "given": 1})

            # Device status every 60 s
            now = time.time()
            if now - last_status >= 60:
                publish("status", {"uptime_s": int(now - start), "wifi_rssi_dbm": random.randint(-70, -50)})
                last_status = now

            time.sleep(args.interval)
    except KeyboardInterrupt:
        print("Stopped.")
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == "__main__":
    main()

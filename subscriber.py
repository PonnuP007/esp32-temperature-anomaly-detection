import paho.mqtt.client as mqtt
import json
import numpy as np
from collections import deque
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from datetime import datetime
import threading

BROKER = "broker.hivemq.com"
PORT = 1883
TOPIC = "ponnu/temperature"
ALERT_TOPIC = "ponnu/alerts"

# Store readings
readings = deque(maxlen=50)
timestamps = deque(maxlen=50)
anomaly_flags = deque(maxlen=50)

client = mqtt.Client()

def detect_anomaly(temp):
    if len(readings) < 5:
        return False
    mean = np.mean(readings)
    std = np.std(readings)
    if std == 0:
        return False
    return abs(temp - mean) > 2 * std

def on_connect(c, userdata, flags, rc):
    if rc == 0:
        print("Connected to MQTT broker!")
        c.subscribe(TOPIC)
        print(f"Subscribed to: {TOPIC}")
    else:
        print(f"Connection failed: {rc}")

def on_message(c, userdata, msg):
    try:
        data = json.loads(msg.payload.decode())
        temp = data["temperature"]
        hum = data["humidity"]

        is_anomaly = detect_anomaly(temp)
        readings.append(temp)
        timestamps.append(datetime.now().strftime("%H:%M:%S"))
        anomaly_flags.append(is_anomaly)

        status = "⚠️  ANOMALY DETECTED!" if is_anomaly else "✅ Normal"
        print(f"Temp: {temp}°C  Humidity: {hum}%  → {status}")

        if is_anomaly:
            mean = np.mean(list(readings)[:-1])
            std = np.std(list(readings)[:-1])
            print(f"   Mean: {mean:.1f}°C  StdDev: {std:.1f}")

            # Publish alert back via MQTT
            alert = json.dumps({
                "alert": "ANOMALY",
                "temperature": temp,
                "mean": round(float(mean), 1),
                "std": round(float(std), 1),
                "timestamp": datetime.now().isoformat()
            })
            c.publish(ALERT_TOPIC, alert)
            print(f"   🚨 Alert published to {ALERT_TOPIC}")

    except Exception as e:
        print(f"Error: {e}")

# ── Matplotlib live plot ──────────────────────────────────────────────────────

fig, ax = plt.subplots(figsize=(10, 5))

def animate(i):
    ax.clear()
    if len(readings) == 0:
        return

    temps = list(readings)
    times = list(timestamps)
    flags = list(anomaly_flags)

    # Plot normal readings
    normal_x = [t for t, f in zip(times, flags) if not f]
    normal_y = [v for v, f in zip(temps, flags) if not f]

    # Plot anomalies
    anomaly_x = [t for t, f in zip(times, flags) if f]
    anomaly_y = [v for v, f in zip(temps, flags) if f]

    ax.plot(times, temps, 'b-', linewidth=1.5, label='Temperature')
    ax.scatter(normal_x, normal_y, color='green', s=30, zorder=5)
    ax.scatter(anomaly_x, anomaly_y, color='red', s=100,
               marker='X', zorder=5, label='Anomaly')

    # Draw mean and threshold lines
    if len(temps) >= 5:
        mean = np.mean(temps)
        std = np.std(temps)
        ax.axhline(mean, color='orange', linestyle='--',
                   linewidth=1, label=f'Mean: {mean:.1f}°C')
        ax.axhline(mean + 2*std, color='red', linestyle=':',
                   linewidth=1, label=f'+2σ: {mean+2*std:.1f}°C')
        ax.axhline(mean - 2*std, color='red', linestyle=':',
                   linewidth=1, label=f'-2σ: {mean-2*std:.1f}°C')

    ax.set_title('Temperature Anomaly Detection - Live Monitor')
    ax.set_xlabel('Time')
    ax.set_ylabel('Temperature (°C)')
    ax.legend(loc='upper left')
    ax.tick_params(axis='x', rotation=45)
    plt.tight_layout()

def mqtt_thread():
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(BROKER, PORT, 60)
    client.loop_forever()

# ── Start ─────────────────────────────────────────────────────────────────────

print("Starting Temperature Anomaly Detection System...")
thread = mqtt.Client.__new__(mqtt.Client)
t = threading.Thread(target=mqtt_thread, daemon=True)
t.start()

ani = animation.FuncAnimation(fig, animate, interval=2000)
plt.show()
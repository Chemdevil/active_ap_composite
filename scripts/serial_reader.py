import serial
import time
import sys
import datetime
import argparse

# Configuration parameters
SERIAL_PORT = 'COM4'  # Change to your serial port
BAUD_RATE = 921600      # Adjust baud rate as needed
TIMEOUT = 5           # Timeout in seconds
DISCONNECT_DELAY = 2  # Time to wait before trying to reconnect (in seconds)


def parse_args():
    parser = argparse.ArgumentParser(description="Read CSI data from serial and log to CSV.")
    parser.add_argument(
        "--file_path",
        required=True,
        type=str,
        help="Output file path (e.g., sitting_idle.csv)"
    )
    parser.add_argument(
        "--seconds",
        type=int,
        default=3,
        help="Total seconds to record (default: 3)"
    )
    parser.add_argument(
        "--port",
        type=str,
        required=True,
        default=3,
        help="Port"
    )
    return parser.parse_args()

def read_serial_port(port, baudrate, timeout):
    try:
        ser = serial.Serial(port, baudrate, timeout=timeout)
        print(f"Connected to {port} at {baudrate} baud.")
        return ser
    except serial.SerialException as e:
        print(f"Error: {e}")
        return None

def main():
    args = parse_args()
    total_seconds = args.seconds
    SERIAL_PORT = args.port
    ser = read_serial_port(SERIAL_PORT, BAUD_RATE, TIMEOUT)
    
    if ser is None:
        sys.exit("Failed to connect to the serial port. Exiting...")

    file_path = args.file_path
    file_object = open("{file_path}_{time_string}.csv".format(file_path=file_path,time_string=datetime.datetime.now().strftime("%Y-%m-%d_%H_%M")),"a")
    start_time = datetime.datetime.now()
    print("Start time:", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"))
    file_object.write("Start time:" + " " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f") + "\n")
    content = ""
    last_data_time = time.time()


    while True and datetime.datetime.now() - start_time < datetime.timedelta(seconds=total_seconds):
        try:
            if ser.in_waiting > 0:
                data = ser.readline()
                try:
                    data = data.decode("utf-8",errors='ignore')
                    if data:
                        data = str(data).replace("\r","").replace("\n","")
                        if "CSI_DATA" in data:
                            if "Start time file" not in content:
                                content+="Start time file:" + " " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f") + "\n"
                                file_object.write("Start time file:" + " " + datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f") + "\n")
                            file_object.write(data.strip() + "\n")
                            content+=data
                        last_data_time = time.time()
                except UnicodeDecodeError:
                    pass


            else:
                current_time = time.time()
                if current_time - last_data_time > TIMEOUT:
                    print("No data received. Attempting to reconnect...")
                    ser.close()
                    time.sleep(DISCONNECT_DELAY)
                    ser = read_serial_port(SERIAL_PORT, BAUD_RATE, TIMEOUT)
                    last_data_time = time.time()
        except serial.SerialException as e:
            print(f"Serial Exception: {e}")
            ser.close()
            time.sleep(DISCONNECT_DELAY)
            ser = read_serial_port(SERIAL_PORT, BAUD_RATE, TIMEOUT)
        except UnicodeDecodeError:
            # Handle the case where data cannot be decoded
            print(f"Received non-UTF-8 data: {temp}")
    print("End time:", datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"))
    exit()

if __name__ == "__main__":
    main()

import subprocess
import time

file_name = "rikshita_trailer_h"

total_duration = 40


# Start the first script
start_time_1 = time.perf_counter()
process1 = subprocess.Popen(['python', r'E:\Older_Data\Esp-32-CSI-Collection\serial_reader.py',
    '--file_path', f'{file_name}.csv',
    '--seconds', f'{total_duration}',
    '--port', 'COM4'])

# Start the second script
start_time_2 = time.perf_counter()
process2 = subprocess.Popen(['python', r'E:\Older_Data\Esp-32-CSI-Collection\video_recording.py','--file_name', f'{file_name}.mp4',  # or '-f', 'a.mp4'
    '--duration', f'{total_duration}',
    '--fps', '30',           # Add if required (common default)
    '--resolution', '1920', '1080'  # Width Height (common default)
    ])

# Wait for both scripts to complete
process1.wait()
process2.wait()

out1, err1 = process1.communicate()
out2, err2 = process2.communicate()

# Extract timestamps from output
#csi_time = float(out1.split("started at")[1].split()[0])
#video_time = float(out2.split("started at")[1].split()[0])

launch_diff = start_time_2 - start_time_1
print(f"Precise time difference between launching the two scripts: {launch_diff:.9f} seconds")

#diff = abs(video_time - csi_time)
#print(f"Time difference between camera and CSI start: {diff:.9f} seconds")
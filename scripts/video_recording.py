from datetime import datetime
import cv2
import time
import argparse
import os

def record_video(duration_seconds, output_filename, fps=30, resolution=(1280, 720)):
    """
    Records a video for a given time duration and saves it in MP4 format.
    """
    # Open the video capture (camera or external device)
    cap = cv2.VideoCapture(0)  # 0 for default webcam, change if needed

    if not cap.isOpened():
        print("Error: Could not open webcam.")
        return

    # Set video properties BEFORE creating writer
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, resolution[0])
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, resolution[1])
    cap.set(cv2.CAP_PROP_FPS, fps)
    
    # Wait for camera to stabilize
    time.sleep(1)
    
    # Test actual resolution
    actual_width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
    actual_height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))
    actual_fps = cap.get(cv2.CAP_PROP_FPS)
    print(f"Camera actual: {actual_width}x{actual_height} @ {actual_fps:.1f}fps")
    
    # FIXED: Multiple codec fallbacks + validation
    codecs = ['mp4v', 'XVID', 'H264', 'MJPG']
    out = None
    
    for codec_str in codecs:
        fourcc = cv2.VideoWriter_fourcc(*codec_str)
        out = cv2.VideoWriter(output_filename, fourcc, fps, (actual_width, actual_height))
        if out.isOpened():
            print(f"✓ Using codec: {codec_str}")
            break
        out.release()
    
    if not out or not out.isOpened():
        print("ERROR: No working codec found!")
        cap.release()
        return

    frame_count = 0
    start_time = time.time()
    
    print("Recording... Press 'q' to stop early")

    while time.time() - start_time < duration_seconds:
        ret, frame = cap.read()
        if not ret:
            print("Failed to capture frame.")
            continue  # Skip bad frames instead of breaking

        # Flip frame (horizontal mirror)
        frame = cv2.flip(frame, 1)
        
        # FIXED: Only write valid frames
        out.write(frame)
        frame_count += 1
        
        # Show frame for monitoring
        cv2.imshow('Recording...', frame)
        
        # Check for early exit
        if cv2.waitKey(1) & 0xFF == ord('q'):
            print("Recording stopped early.")
            break

    # Proper cleanup
    cap.release()
    out.release()
    cv2.destroyAllWindows()
    
    print(f"✓ Recording finished. Saved {output_filename}")
    print(f"✓ Total frames: {frame_count}, Duration: {time.time()-start_time:.1f}s")

def append_timestamp(file_path):
    """Automatically appends timestamp to filename before extension"""
    base, ext = os.path.splitext(file_path)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    return f"{base}_{timestamp}{ext}"

def parse_args():
    parser = argparse.ArgumentParser(description="Record video from webcam")
    parser.add_argument("--file_name", "-f", required=True,
                       help="Base output video file name (timestamp will be auto-appended)")
    parser.add_argument("--duration", "-d", type=int, default=180,
                       help="Recording duration in seconds (default: 180)")
    parser.add_argument("--fps", "-r", type=int, default=30,
                       help="Frames per second (default: 30)")
    parser.add_argument("--resolution", "-res", nargs=2, type=int, default=[1280, 720],
                       help="Resolution width height (default: 1280 720)")
    return parser.parse_args()

def main():
    args = parse_args()
    output_filename = append_timestamp(args.file_name)
    
    print(f"Recording to: {output_filename}")
    print(f"Duration: {args.duration}s, FPS: {args.fps}, Res: {args.resolution[0]}x{args.resolution[1]}")
    
    record_video(
        duration_seconds=args.duration,
        output_filename=output_filename,
        fps=args.fps,
        resolution=tuple(args.resolution)
    )

if __name__ == "__main__":
    main()

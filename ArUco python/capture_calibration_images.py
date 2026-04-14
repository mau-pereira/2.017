import os

import cv2


def main() -> None:
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(base_dir, "calibration_images")
    os.makedirs(output_dir, exist_ok=True)

    cap = cv2.VideoCapture(1)
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)

    if not cap.isOpened():
        print("Error: could not open webcam.")
        return

    cv2.namedWindow("Calibration Capture", cv2.WINDOW_NORMAL)
    cv2.resizeWindow("Calibration Capture", 480, 270)

    print(f'Saving all images to this folder "{output_dir}"')
    print("Press 's' to save image, 'q' to quit.")

    existing_images = [
        name for name in os.listdir(output_dir)
        if name.lower().startswith("img") and name.lower().endswith(".jpg")
    ]
    image_count = len(existing_images)
    while True:
        ok, frame = cap.read()
        if not ok:
            print("Error: could not read frame from webcam.")
            break

        cv2.imshow("Calibration Capture", frame)
        key = cv2.waitKey(1) & 0xFF

        if key == ord("s"):
            filename = f"img{image_count + 1}.jpg"
            save_path = os.path.join(output_dir, filename)
            cv2.imwrite(save_path, frame)
            image_count += 1
            print(f"Saved image {image_count}")
        elif key == ord("q"):
            break

    cap.release()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()

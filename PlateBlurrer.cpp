#include "PlateBlurrer.h"

using namespace std;
namespace fs = std::filesystem;

void blurLicensePlate(cv::Mat& image, const cv::Rect& plate) {
    cv::Rect safeBox = plate & cv::Rect(0, 0, image.cols, image.rows);
    if (safeBox.width <= 0 || safeBox.height <= 0) return;

    cv::Mat roi = image(safeBox);
	// Apply a strong Gaussian blur to the detected license plate region
    cv::GaussianBlur(roi, roi, cv::Size(75, 75), 0);
}

int main() {
	//Disables logging from OpenCV to keep console output clean
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);
#if defined(_WIN32) || defined(_WIN64)
    // 1. Set Console Title
    SetConsoleTitleA("Media Plate Blurrer");

    // 2. Force Windows Console to process ANSI Color Codes
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#endif

    printCentered(COLOR_BOLD "=== Video Plate Blurrer Initializing ===" COLOR_RESET);

    std::string choice;
    do {
        printCentered(COLOR_CYAN "Select Input Type:" COLOR_RESET);
        printCentered("1. Process a Video (.mp4)");
        printCentered("2. Process a Photo (.jpg / .png)");
        printCentered(COLOR_YELLOW "[Choice (1 or 2)] = " COLOR_RESET);
        std::getline(std::cin, choice);

        if (choice != "1" && choice != "2") {
            printCentered(COLOR_RED "[Error] Invalid choice. Please enter '1' or '2'." COLOR_RESET);
		}
    }while (choice != "1" && choice != "2");

    // 2. File Verification Loop
    std::string inputMediaName;
    bool fileFound = false;
    while (!fileFound) {
        if (fs::exists(inputMediaName) && !fs::is_directory(inputMediaName)) {
            fileFound = true;
        }
        else {
            if (!inputMediaName.empty()) {
                printCentered(COLOR_RED "[Error] File not found or invalid." COLOR_RESET);
            }
            printCentered(COLOR_YELLOW "[Input] Type file name with extension: " COLOR_RESET);
            std::getline(std::cin, inputMediaName);
        }
    }

    if (!fs::exists("output")) {
        fs::create_directory("output");
        printCentered(COLOR_GREEN "[Notice] Created missing 'output' directory." COLOR_RESET);
    }

    // ONNX model
    std::string modelPath = "plate_detector.onnx";
    cv::dnn::Net net;
    try {
        net = cv::dnn::readNetFromONNX(modelPath);
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        printCentered(COLOR_GREEN "[Notice] ONNX Model loaded successfully." COLOR_RESET);
    }
    catch (const cv::Exception& e) {
        printCentered(COLOR_RED "[Error] Failed to load ONNX model: " COLOR_RESET);
        return -1;
    }

    // =================================================================
    // PHOTO PROCESSING
    // =================================================================
    if (choice == "2") {
        printCentered(COLOR_YELLOW "[Notice] Processing photo... please wait..." COLOR_RESET);

        cv::Mat image = cv::imread(inputMediaName);
        if (image.empty()) {
            printCentered(COLOR_RED "[Error] Could not decode image file." COLOR_RESET);
            return -1;
        }

        // Pre-process image to 4D blob
        cv::Mat blob;
        cv::dnn::blobFromImage(image, blob, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);
        net.setInput(blob);

        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        cv::Mat outputMatrix = outputs[0];
        if (outputMatrix.dims == 3 && outputMatrix.size[1] == 5) {
            outputMatrix = outputMatrix.reshape(1, outputMatrix.size[1]);
            cv::transpose(outputMatrix, outputMatrix);
        }

        float x_scale = (float)image.cols / 640.0f;
        float y_scale = (float)image.rows / 640.0f;

        // Extract plates found in the single frame
        for (int i = 0; i < outputMatrix.rows; ++i) {
            float confidence = outputMatrix.at<float>(i, 4);
            if (confidence > 0.45f) {
                float cx = outputMatrix.at<float>(i, 0);
                float cy = outputMatrix.at<float>(i, 1);
                float w = outputMatrix.at<float>(i, 2);
                float h = outputMatrix.at<float>(i, 3);

                int x = static_cast<int>((cx - w / 2.0f) * x_scale);
                int y = static_cast<int>((cy - h / 2.0f) * y_scale);
                int width = static_cast<int>(w * x_scale);
                int height = static_cast<int>(h * y_scale);

                blurLicensePlate(image, cv::Rect(x, y, width, height));
            }
        }

        std::string outputPath = "output/" + inputMediaName;
        cv::imwrite(outputPath, image);
        printCentered(COLOR_GREEN "[Success] Processed photo saved to: " + outputPath + COLOR_RESET);
        return 0; // Execution ends successfully here for photos
    }

    // =================================================================
    // VIDEO PROCESSING
    // =================================================================
    cv::VideoCapture cap(inputMediaName);
    if (!cap.isOpened()) {
        printCentered(COLOR_RED "[Error] Could not open the video file '" + inputMediaName + "'!" COLOR_RESET);
        return -1;
    }

    int frameWidth = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frameHeight = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    double fps = cap.get(cv::CAP_PROP_FPS);
    int totalFrames = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    printCentered(COLOR_YELLOW "[Notice] Processing video: " COLOR_RESET);
    printCentered(std::to_string(frameWidth) + "x" + std::to_string(frameHeight) + " | FPS: " + std::to_string(fps));

    cv::VideoWriter writer(
        "output/" + inputMediaName,
        cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
        fps,
        cv::Size(frameWidth, frameHeight)
    );

    if (!writer.isOpened()) {
        printCentered(COLOR_RED "[Error] Could not initialize VideoWriter stream output file!" COLOR_RESET);
        return -1;
    }

    cv::Rect lastKnownPlateBox(0, 0, 0, 0);
    int persistenceFramesCounter = 0;
    const int MAX_PERSISTENCE_FRAMES = 8;

    cv::Mat frame;
    int currentFrameIndex = 0;

    printCentered(COLOR_YELLOW "[Notice] Processing background rendering pipeline... please wait..." COLOR_RESET);

    while (cap.read(frame)) {
        if (frame.empty()) break;
        currentFrameIndex++;

        if (currentFrameIndex % 30 == 0 || currentFrameIndex == totalFrames) {
            printCentered("Progress: " + std::to_string(currentFrameIndex) + " / " + std::to_string(totalFrames) + " frames saved.");
        }

        cv::Mat blob;
        cv::dnn::blobFromImage(frame, blob, 1.0 / 255.0, cv::Size(640, 640), cv::Scalar(), true, false);

        net.setInput(blob);
        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());

        cv::Mat outputMatrix = outputs[0];
        if (outputMatrix.dims == 3 && outputMatrix.size[1] == 5) {
            outputMatrix = outputMatrix.reshape(1, outputMatrix.size[1]);
            cv::transpose(outputMatrix, outputMatrix);
        }

        float x_scale = (float)frame.cols / 640.0f;
        float y_scale = (float)frame.rows / 640.0f;

        bool plateFoundThisFrame = false;
        float highestConfidenceThisFrame = 0.0f;
        cv::Rect bestBoxThisFrame(0, 0, 0, 0);

        for (int i = 0; i < outputMatrix.rows; ++i) {
            float confidence = outputMatrix.at<float>(i, 4);

            if (confidence > 0.45f) {
                if (confidence > highestConfidenceThisFrame) {
                    highestConfidenceThisFrame = confidence;
                    plateFoundThisFrame = true;

                    float cx = outputMatrix.at<float>(i, 0);
                    float cy = outputMatrix.at<float>(i, 1);
                    float w = outputMatrix.at<float>(i, 2);
                    float h = outputMatrix.at<float>(i, 3);

                    int x = static_cast<int>((cx - w / 2.0f) * x_scale);
                    int y = static_cast<int>((cy - h / 2.0f) * y_scale);
                    int width = static_cast<int>(w * x_scale);
                    int height = static_cast<int>(h * y_scale);

                    bestBoxThisFrame = cv::Rect(x, y, width, height);
                }
            }
        }

        if (plateFoundThisFrame) {
            lastKnownPlateBox = bestBoxThisFrame;
            persistenceFramesCounter = MAX_PERSISTENCE_FRAMES;
        }
        else {
            if (persistenceFramesCounter > 0) {
                persistenceFramesCounter--;
            }
        }

        if (persistenceFramesCounter > 0 && lastKnownPlateBox.width > 0) {
            blurLicensePlate(frame, lastKnownPlateBox);
        }

        writer.write(frame);
    }

    cap.release();
    writer.release();
    cv::destroyAllWindows();

    printCentered(COLOR_GREEN "[Success] Complete video file saved to: output/" + inputMediaName + COLOR_RESET);
    return 0;
}
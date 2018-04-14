#include "rainbowDice.hpp"

void RainbowDice::initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "RainbowDice", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, RainbowDice::onWindowResized);
    glfwSetMouseButtonCallback(window, RainbowDice::mouseButtonCallback);
}

void RainbowDice::mainLoop() {
    while (!glfwWindowShouldClose(window)) {
        //glfwWaitEvents();
        glfwPollEvents();

        updateUniformBuffer();

        drawFrame();
    }

    /* wait for all drawing operations to complete */
    vkDeviceWaitIdle(logicalDevice);
}

void RainbowDice::destroyWindow() {
    glfwDestroyWindow(window);

    glfwTerminate();
}

void RainbowDice::startRotateImage() {
    xprev = INVALID_MOUSE_POSITION;
    yprev = INVALID_MOUSE_POSITION;
    xstart = INVALID_MOUSE_POSITION;
    ystart = INVALID_MOUSE_POSITION;
    glfwSetCursorPosCallback(window, rotateCallback);
}

void RainbowDice::startDragImage() {
    xprevdrag = INVALID_MOUSE_POSITION;
    yprevdrag = INVALID_MOUSE_POSITION;
    glfwSetCursorPosCallback(window, dragCallback);
}

void RainbowDice::stopRotateImage() {
    xprev = INVALID_MOUSE_POSITION;
    yprev = INVALID_MOUSE_POSITION;
    xstart = INVALID_MOUSE_POSITION;
    ystart = INVALID_MOUSE_POSITION;
    glfwSetCursorPosCallback(window, nullptr);
}

void RainbowDice::stopDragImage() {
    xprevdrag = INVALID_MOUSE_POSITION;
    yprevdrag = INVALID_MOUSE_POSITION;
    totalX = 0;
    totalY = 0;
    glfwSetCursorPosCallback(window, nullptr);
}

void RainbowDice::rotate(double xpos, double ypos) {
    if (xprev == INVALID_MOUSE_POSITION || yprev == INVALID_MOUSE_POSITION) {
        xprev = xstart = xpos;
        yprev = ystart = ypos;
        return;
    }

    xprev = xpos;
    yprev = ypos;
}

void RainbowDice::drag(double xpos, double ypos) {
    if (xprevdrag == INVALID_MOUSE_POSITION || yprevdrag == INVALID_MOUSE_POSITION) {
        xprevdrag = xpos;
        yprevdrag = ypos;
        return;
    }

    totalY = ypos - yprevdrag;
    totalX = xpos - xprevdrag;
    xprevdrag = xpos;
    yprevdrag = ypos;
}

void RainbowDice::onWindowResized(GLFWwindow *window, int width, int height) {
    RainbowDice *app = reinterpret_cast<RainbowDice*>(glfwGetWindowUserPointer(window));
    app->recreateSwapChain();
    app->updatePerspectiveMatrix();
}

void RainbowDice::rotateCallback(GLFWwindow* window, double xpos, double ypos) {
    RainbowDice *app = reinterpret_cast<RainbowDice*>(glfwGetWindowUserPointer(window));
    app->rotate(xpos, ypos);
}

void RainbowDice::dragCallback(GLFWwindow* window, double xpos, double ypos) {
    RainbowDice *app = reinterpret_cast<RainbowDice*>(glfwGetWindowUserPointer(window));
    app->drag(xpos, ypos);
}

void RainbowDice::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    RainbowDice *app = reinterpret_cast<RainbowDice*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        app->startRotateImage();
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        app->startDragImage();
    } else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
        app->stopRotateImage();
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        app->stopDragImage();
    }
}

void RainbowDice::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

/* read the shader byte code files */
std::vector<char> RainbowDice::readFile(const std::string &filename) {
    /* std::ios::ate means start reading at the end of the file */
    std::ifstream filein(filename, std::ios::in | std::ios::ate | std::ios::binary);

    if (!filein.is_open()) {
        throw std::runtime_error("failed to open file: " + filename);
    }

    size_t fileSize = (size_t) filein.tellg();
    std::vector<char> buffer(fileSize);

    filein.seekg(0);
    filein.read(buffer.data(), fileSize);

    return buffer;
}

#include "render/render.hpp"

int main() {
    vkengine::init_engine();
    vkengine::run();
    vkengine::cleanup();
}

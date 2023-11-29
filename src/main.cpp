#include "vk_engine.h"

int main(int argc, char *argv[])
{
    vk_engine engine = {};
    engine.init();
    engine.run();
    engine.cleanup();
    return 0;
}

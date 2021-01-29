set(COMPONENT_SRCS
        src/IotWebConf.cpp
        src/IotWebConfCompatibility.cpp
        )

set(COMPONENT_ADD_INCLUDEDIRS
        src/
        )
list(APPEND COMPONENT_REQUIRES "arduino")
register_component()

ADD_DEFINITIONS(-DESP32)

list(APPEND DEFINITIONS "ESP32")


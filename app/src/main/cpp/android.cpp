/**
 * Copyright 2018 Cerulean Quasar. All Rights Reserved.
 *
 *  This file is part of AmazingLabyrinth.
 *
 *  AmazingLabyrinth is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  AmazingLabyrinth is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with AmazingLabyrinth.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include "android_native_app_glue.h"
#include <string>
#include <asm/fcntl.h>
#include <istream>
#include <vector>

#include "android.hpp"

std::unique_ptr<AssetManagerWrapper> assetWrapper;

int const Sensors::MAX_EVENT_REPORT_TIME = 20000;
int const Sensors::EVENT_TYPE_ACCELEROMETER = 462;

void Sensors::initSensors() {
    sensorManager = ASensorManager_getInstance();
    sensor = ASensorManager_getDefaultSensor(sensorManager, ASENSOR_TYPE_ACCELEROMETER);
    if (sensor == nullptr) {
        // TODO: use a flick gesture instead?
        throw std::runtime_error("Accelerometer not present.");
    }

    looper = ALooper_forThread();
    if (looper == nullptr) {
        looper = ALooper_prepare(0);
    }

    if (looper == nullptr) {
        throw std::runtime_error("Could not initialize looper.");
    }

    eventQueue = ASensorManager_createEventQueue(sensorManager, looper, EVENT_TYPE_ACCELEROMETER, nullptr, nullptr);

    int rc = ASensorEventQueue_enableSensor(eventQueue, sensor);
    if (rc < 0) {
        throw std::runtime_error("Could not enable sensor");
    }
    int minDelay = ASensor_getMinDelay(sensor);
    minDelay = std::max(minDelay, MAX_EVENT_REPORT_TIME);

    rc = ASensorEventQueue_setEventRate(eventQueue, sensor, minDelay);
    if (rc < 0) {
        ASensorEventQueue_disableSensor(eventQueue, sensor);
        throw std::runtime_error("Could not set event rate");
    }
}

void setAssetManager(AAssetManager *mgr) {
    assetWrapper.reset(new AssetManagerWrapper(mgr));
}

std::unique_ptr<AAsset> AssetManagerWrapper::getAsset(std::string const &path) {
    AAsset *asset = AAssetManager_open(manager, path.c_str(), O_RDONLY);
    if (asset == nullptr) {
        throw std::runtime_error(std::string("File not found: ") + path);
    }
    return std::unique_ptr<AAsset>(asset);
}

AssetStreambuf::int_type AssetStreambuf::underflow() {
    int bytesRead = AAsset_read(asset.get(), buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        return traits_type::eof();
    }

    setg(buffer, buffer, buffer + bytesRead);
    return traits_type::to_int_type(buffer[0]);
}

std::streampos AssetStreambuf::seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which) {
    if (which != std::ios_base::in) {
        return -1;
    }
    off64_t offset = off;
    int whence;
    switch (way) {
        case std::ios_base::beg :
            whence = SEEK_SET;
            break;
        case std::ios_base::cur :
            whence = SEEK_CUR;
            break;
        case std::ios_base::end :
            whence = SEEK_END;
            break;
        default:
            return -1;
    }
    offset = AAsset_seek64(asset.get(), offset, whence);

    return offset;
}

std::streampos AssetStreambuf::seekpos(std::streampos pos, std::ios_base::openmode which) {
    return seekoff(pos, std::ios_base::beg, which);
}

std::vector<char> readFile(std::string const &filename) {
    AssetStreambuf assetStreambuf(assetWrapper->getAsset(filename));
    std::istream reader(&assetStreambuf);
    std::vector<char> data;
    unsigned long const readSize = 1024;

    while (!reader.eof()) {
        unsigned long size = data.size();
        data.resize(size + readSize);
        long bytesRead = reader.read(data.data()+size, readSize).gcount();

        if (bytesRead != readSize) {
            data.resize(size + bytesRead);
        }
    }

    return data;
}

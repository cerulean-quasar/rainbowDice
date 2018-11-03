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
#ifndef AMAZING_LABYRINTH_ANDROID_HPP
#define AMAZING_LABYRINTH_ANDROID_HPP

#include <native_window.h>
#include <asset_manager.h>
#include <asset_manager_jni.h>
#include <memory>
#include <streambuf>

typedef ANativeWindow WindowType;

namespace std {
    template<> class default_delete<AAsset> {
    public:
        void operator()(AAsset *toBeDeleted) {
            AAsset_close(toBeDeleted);
        }
    };
}

class AssetStreambuf : public std::streambuf {
private:
    std::unique_ptr<AAsset> asset;
    char buffer[4096];
public:
    explicit AssetStreambuf(std::unique_ptr<AAsset> &&inAsset) : asset(std::move(inAsset)) {}
    int underflow();
    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);
    std::streampos seekpos(std::streampos pos, std::ios_base::openmode which);
};

class AssetManagerWrapper {
private:
    AAssetManager *manager;
public:
    AssetManagerWrapper(AAssetManager *inManager) : manager(inManager) {}
    std::unique_ptr<AAsset> getAsset(std::string const &file);
};

extern std::unique_ptr<AssetManagerWrapper> assetWrapper;

std::vector<char> readFile(std::string const &filename);
void setAssetManager(AAssetManager *mgr);

#endif
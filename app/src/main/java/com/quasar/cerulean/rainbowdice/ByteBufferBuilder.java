/**
 * Copyright 2019 Cerulean Quasar. All Rights Reserved.
 *
 *  This file is part of RainbowDice.
 *
 *  RainbowDice is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  RainbowDice is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with RainbowDice.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
package com.quasar.cerulean.rainbowdice;

import java.io.IOException;
import java.io.InputStream;

public class ByteBufferBuilder {
    private byte[] bytes;
    private int sizeOfData;
    public ByteBufferBuilder(int estimatedSize) {
        bytes = new byte[estimatedSize];
        sizeOfData = 0;
    }

    public byte[] getBytes() {
        if (bytes.length == sizeOfData) {
            return bytes;
        } else {
            byte[] data = new byte[sizeOfData];
            System.arraycopy(bytes,0, data, 0, sizeOfData);
            return data;
        }
    }

    public void readToBuffer(InputStream in, boolean nullAppend) throws IOException {
        int len = 0;
        do {
            if (bytes.length - sizeOfData > 0) {
                len = in.read(bytes, sizeOfData, bytes.length - sizeOfData);
                if (len > 0) {
                    sizeOfData += len;
                }
            } else {
                byte[] newBytes = new byte[bytes.length * 2];
                System.arraycopy(bytes, 0, newBytes, 0, bytes.length);
                bytes = newBytes;
            }
        } while (len >= 0);
        if (nullAppend) {
            if (bytes.length - sizeOfData > 0) {
                bytes[sizeOfData] = '\0';
            } else {
                byte[] newBytes = new byte[bytes.length + 1];
                System.arraycopy(bytes, 0, newBytes, 0, bytes.length);
                bytes = newBytes;
                bytes[sizeOfData] = '\0';
            }
            sizeOfData++;
        }
    }
}

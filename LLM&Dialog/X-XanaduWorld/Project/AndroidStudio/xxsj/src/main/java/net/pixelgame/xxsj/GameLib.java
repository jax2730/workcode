/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package net.pixelgame.xxsj;

// Wrapper for native library

public class GameLib {
    /**
     * @param width the current view width
     * @param height the current view height
     */
     public static native void init( String[] variables);
	 public static native void resize( int width, int height );
     public static native void step();
	 public static native void pause();
	 public static native void resume();
	 public static native void terminate();
	 public static native void onTouch( int _action, int _touchId, int _touchCount, float _x, float _y );
	 public static native void onTextProc( String _text );
	 public static native int getFPS();
	 public static native int getDrawCall();
	 public static native int getTriangleCnt();
	 public static native String getCameraPosition();
	 public static native String getResMemUsageStr();
	 public static native int getEchoRenderMode();
	 public static native void nextScene();
	 public static native void lastScene();
	 public static native void camModeChange();
	 public static native void camSpeedUp();
	 public static native void camSpeedReduction();

}

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
/*
 * Copyright (C) 2008 The Android Open Source Project
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

import android.content.Context;
import android.graphics.Point;
import android.os.Build;
import android.util.DisplayMetrics;
import android.view.Display;
import android.view.WindowManager;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class GameUtils {    
      
	public static List<String> getMemInfo() {
        List<String> result = new ArrayList<String>();

        try {
            String line;
            BufferedReader br = new BufferedReader(new FileReader("/proc/meminfo"));
            while ((line = br.readLine()) != null) {
                result.add(line);
            }
            br.close();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return result;
    }

    /* /proc/meminfo

        MemTotal:        2902436 kB
        MemFree:          199240 kB
        MemAvailable:    1088764 kB
        Buffers:           40848 kB
        Cached:           862908 kB
        SwapCached:        54696 kB
        Active:          1222848 kB
        Inactive:         671468 kB
        Active(anon):     758516 kB
        Inactive(anon):   242560 kB
        Active(file):     464332 kB
        Inactive(file):   428908 kB
        Unevictable:        5972 kB
        Mlocked:             256 kB
        SwapTotal:       1048572 kB
        SwapFree:         537124 kB
        Dirty:                12 kB
        Writeback:             0 kB
        AnonPages:        988820 kB
        Mapped:           508996 kB
        Shmem:              4800 kB
        Slab:             157204 kB
        SReclaimable:      57364 kB
        SUnreclaim:        99840 kB
        KernelStack:       41376 kB
        PageTables:        51820 kB
        NFS_Unstable:          0 kB
        Bounce:                0 kB
        WritebackTmp:          0 kB
        CommitLimit:     2499788 kB
        Committed_AS:   76292324 kB
        VmallocTotal:   258867136 kB
        VmallocUsed:           0 kB
        VmallocChunk:          0 kB
        CmaTotal:              0 kB
        CmaFree:               0 kB
    */
    public static String getFieldFromMeminfo(String field) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader("/proc/meminfo"));
        Pattern p = Pattern.compile(field + "\\s*:\\s*(.*)");

        try {
            String line;
            while ((line = br.readLine()) != null) {
                Matcher m = p.matcher(line);
                if (m.matches()) {
                    return m.group(1);
                }
            }
        } finally {
            br.close();
        }

        return null;
    }

    public static String getMemTotal() {
        String result = null;

        try {
            result = getFieldFromMeminfo("MemTotal");
        } catch (IOException e) {
            e.printStackTrace();
        }

        return result;
    }


    public static String getMemAvailable() {
        String result = null;

        try {
            result = getFieldFromMeminfo("MemAvailable");
        } catch (IOException e) {
            e.printStackTrace();
        }

        return result;
    }

	public static String getMemFree() {
        String result = null;

        try {
            result = getFieldFromMeminfo("MemFree");
        } catch (IOException e) {
            e.printStackTrace();
        }

        return result;
    }

    
    /**
     *
     * @param mContext
     * @return
     */
    public static String getScreenInfo(Context mContext) {
        int widthPixels;
        int heightPixels;

        WindowManager w = (WindowManager) mContext.getSystemService(Context.WINDOW_SERVICE);

        Display d = w.getDefaultDisplay();
        DisplayMetrics metrics = new DisplayMetrics();
        d.getMetrics(metrics);
        // since SDK_INT = 1;
        widthPixels = metrics.widthPixels;
        heightPixels = metrics.heightPixels;
        // includes window decorations (statusbar bar/menu bar)
        if (Build.VERSION.SDK_INT >= 14 && Build.VERSION.SDK_INT < 17) {
            try {
                widthPixels = (Integer) Display.class.getMethod("getRawWidth").invoke(d);
                heightPixels = (Integer) Display.class.getMethod("getRawHeight").invoke(d);
            } catch (Exception ignored) {
                ignored.printStackTrace();
            }
        }
        // includes window decorations (statusbar bar/menu bar)
        if (Build.VERSION.SDK_INT >= 17) {
            try {
                Point realSize = new Point();
                Display.class.getMethod("getRealSize", Point.class).invoke(d, realSize);
                widthPixels = realSize.x;
                heightPixels = realSize.y;
            } catch (Exception ignored) {
                ignored.printStackTrace();
            }
        }
		       
        String densityDpiStr = metrics.densityDpi + " dpi";
        double inchesSize = (Math.sqrt(Math.pow(widthPixels, 2) + Math.pow(heightPixels, 2)) / metrics.densityDpi);

        String inchessizeStr = String.format("%.2f", inchesSize);
        
        return "\"ScreenInfo\":	{" + 
               "\"inchesSize\":	\"" + inchessizeStr + "\"," +
               "\"widthPixels\":	\"" + widthPixels + "\"," +
               "\"heightPixels\":	\"" + heightPixels + "\"," +
               "\"densityDpi\":	\"" + densityDpiStr + "\"}";         
            
    }
    
    public static String getMemoryInfo()
    {
    	 return "\"MemoryInfo\":	{" + 
                 "\"MemTotal\":	\"" + getMemTotal() + "\"," +
                 "\"MemFree\":	\"" + getMemFree() + "\"}";    
    }
	
	public static String deviceModel()
	{
        return Build.MODEL;
    }

	public static String getHardware() {
        String result = Build.HARDWARE;
        if (!result.equals("qcom")) {
            return result;
        } else {
            BufferedReader reader = null;

            try {
                reader = new BufferedReader(new FileReader("/proc/cpuinfo"));

                String line;
                while((line = reader.readLine()) != null) {
                    if (line.startsWith("Hardware")) {
                        result = line.substring(line.indexOf(':') + 1);
                    }
                }
            } catch (FileNotFoundException var13) {
                var13.printStackTrace();
            } catch (IOException var14) {
                var14.printStackTrace();
            } finally {
                if (reader != null) {
                    try {
                        reader.close();
                    } catch (IOException var12) {
                        var12.printStackTrace();
                    }
                }

            }
            if (result.startsWith(" "))
            {
                result = result.substring(1);
            }
            if (result.startsWith("Qualcomm Technologies, Inc"))
            {
                result = result.substring(27);
            }
            result = result.toLowerCase();

            return result != null ? result : "(unknown)";
        }
    }
}

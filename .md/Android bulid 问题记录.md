# Android bulid 问题记录

![image-20250905183945932](C:\Users\Pixel\AppData\Roaming\Typora\typora-user-images\image-20250905183945932.png)



ninja: error: Stat(C:/Users/Pixel/.gradle/wrapper/dists/gradle-8.9-bin/90cnw93cvbtalezasaz0blq0a/gradle-8.9-bin/gradle-8.9/caches/transforms-3/c55f3896fe1d406619238fdd1e7796d2/transformed/games-activity-2.0.2/prefab/modules/game-activity_static/libs/android.arm64-v8a/libgame-activity_static.a): Filename longer than 260 characters

问题原因：

Gradle目录未设置正确

![image-20250906105257170](C:\Users\Pixel\AppData\Roaming\Typora\typora-user-images\image-20250906105257170.png)

解决：

![image-20250906104958237](C:\Users\Pixel\AppData\Roaming\Typora\typora-user-images\image-20250906104958237.png)

Gradle
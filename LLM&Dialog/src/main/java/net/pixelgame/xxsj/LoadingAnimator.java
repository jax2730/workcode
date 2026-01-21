package net.pixelgame.xxsj;

import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.view.View;
import android.view.animation.AccelerateDecelerateInterpolator;
import android.widget.TextView;

/**
 * 加载动画辅助类
 * 实现三个点的波浪式淡入淡出动画
 */
public class LoadingAnimator {
    
    private ObjectAnimator dot1Animator;
    private ObjectAnimator dot2Animator;
    private ObjectAnimator dot3Animator;
    
    private TextView dot1;
    private TextView dot2;
    private TextView dot3;
    
    private boolean isAnimating = false;
    
    public LoadingAnimator(View loadingView) {
        dot1 = loadingView.findViewById(R.id.loading_dot1);
        dot2 = loadingView.findViewById(R.id.loading_dot2);
        dot3 = loadingView.findViewById(R.id.loading_dot3);
        
        setupAnimators();
    }
    
    /**
     * 设置动画
     */
    private void setupAnimators() {
        // 第一个点的动画
        dot1Animator = ObjectAnimator.ofFloat(dot1, "alpha", 0.3f, 1.0f, 0.3f);
        dot1Animator.setDuration(1200);
        dot1Animator.setRepeatCount(ValueAnimator.INFINITE);
        dot1Animator.setInterpolator(new AccelerateDecelerateInterpolator());
        dot1Animator.setStartDelay(0);
        
        // 第二个点的动画（延迟200ms）
        dot2Animator = ObjectAnimator.ofFloat(dot2, "alpha", 0.3f, 1.0f, 0.3f);
        dot2Animator.setDuration(1200);
        dot2Animator.setRepeatCount(ValueAnimator.INFINITE);
        dot2Animator.setInterpolator(new AccelerateDecelerateInterpolator());
        dot2Animator.setStartDelay(200);
        
        // 第三个点的动画（延迟400ms）
        dot3Animator = ObjectAnimator.ofFloat(dot3, "alpha", 0.3f, 1.0f, 0.3f);
        dot3Animator.setDuration(1200);
        dot3Animator.setRepeatCount(ValueAnimator.INFINITE);
        dot3Animator.setInterpolator(new AccelerateDecelerateInterpolator());
        dot3Animator.setStartDelay(400);
    }
    
    /**
     * 开始动画
     */
    public void start() {
        if (!isAnimating) {
            isAnimating = true;
            dot1Animator.start();
            dot2Animator.start();
            dot3Animator.start();
        }
    }
    
    /**
     * 停止动画
     */
    public void stop() {
        if (isAnimating) {
            isAnimating = false;
            if (dot1Animator != null) dot1Animator.cancel();
            if (dot2Animator != null) dot2Animator.cancel();
            if (dot3Animator != null) dot3Animator.cancel();
            
            // 重置透明度
            if (dot1 != null) dot1.setAlpha(0.3f);
            if (dot2 != null) dot2.setAlpha(0.3f);
            if (dot3 != null) dot3.setAlpha(0.3f);
        }
    }
    
    /**
     * 是否正在动画
     */
    public boolean isAnimating() {
        return isAnimating;
    }
}

FadeBehavior {
    fadeProperty: "opacity"
    fadeDuration: 200
    outAnimation.duration: targetValue ? 0 : fadeDuration
    inAnimation.duration: targetValue ? fadeDuration : 0
}

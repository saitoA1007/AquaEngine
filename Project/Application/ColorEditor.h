#pragma once
#include <iostream>
#include <cmath>
#include"EngineSource/Math/Vector3.h"

class ColorConverter {
public:

    struct HSV {
        float h, s, v;
    };

    struct RGB {
        float r, g, b;
    };

    /// <summary>
    /// RGBをHSVに変える処理
    /// </summary>
    /// <param name="rgb"></param>
    /// <returns></returns>
    static Vector3 RGBtoHSV(const RGB& rgb);

    /// <summary>
    /// HSVをRGBに変える処理
    /// </summary>
    /// <param name="hsv"></param>
    /// <returns></returns>
    static Vector3 HSVtoRGB(const HSV& hsv);
}; 


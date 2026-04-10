#include"ColorEditor.h"

Vector3 ColorConverter::RGBtoHSV(const RGB& rgb) {
    float max = std::fmaxf(rgb.r, std::fmaxf(rgb.g, rgb.b));
    float min = std::fminf(rgb.r, std::fminf(rgb.g, rgb.b));
    float delta = max - min;

    HSV hsv;
    hsv.v = max;

    if (delta == 0) {
        hsv.h = 0;
        hsv.s = 0;
    } else {
        hsv.s = delta / max;

        if (max == rgb.r) {
            hsv.h = 60.0f * (fmodf(((rgb.g - rgb.b) / delta), 6));
        } else if (max == rgb.g) {
            hsv.h = 60.0f * (((rgb.b - rgb.r) / delta) + 2);
        } else { // max == b
            hsv.h = 60.0f * (((rgb.r - rgb.g) / delta) + 4);
        }

        if (hsv.h < 0) {
            hsv.h += 360;
        }
    }

    return Vector3(hsv.h,hsv.s, hsv.v);
}

Vector3 ColorConverter::HSVtoRGB(const HSV& hsv) {
    float C = hsv.v * hsv.s; // Chroma
    float X = C * (1.0f - abs(fmod(hsv.h / 60.0f, 2.0f) - 1.0f));
    float m = hsv.v - C;

    RGB rgb{};

	if (hsv.h < 60.0f) {
		rgb = RGB(C, X, 0.0f);

	} else if (hsv.h < 120.0f) {
		rgb = RGB(X, C, 0.0f);

	} else if (hsv.h < 180.0f) {
		rgb = RGB(0.0f, C, X);

	} else if (hsv.h < 240.0f) {
		rgb = RGB(0.0f, X, C);

	} else if (hsv.h < 300.0f) {
		rgb = RGB(X, 0.0f, C);

	} else {
		rgb = RGB(C, 0.0f, X);
	}

	return Vector3(rgb.r + m, rgb.g + m, rgb.b + m);
}
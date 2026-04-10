
float32_t3 RGBToHSV(float32_t3 rgb)
{
    float32_t cmax = max(rgb.r, max(rgb.g, rgb.b));
    float32_t cmin = min(rgb.r, min(rgb.g, rgb.b));
    float32_t delta = cmax - cmin;

    float32_t hue = 0.0f; // H
    if (delta > 0.0001f)
    {
        if (cmax == rgb.r)
        {
            hue = fmod((rgb.g - rgb.b) / delta, 6.0f);
        }
        else if (cmax == rgb.g)
        {
            hue = ((rgb.b - rgb.r) / delta) + 2.0f;
        }
        else
        {
            hue = ((rgb.r - rgb.g) / delta) + 4.0f;
        }

        hue *= 60.0f;
        if (hue < 0.0f)
        {
            hue += 360.0f;
        }
    }

    float32_t saturation = (cmax == 0.0f) ? 0.0f : (delta / cmax); // S
    float32_t value = cmax; // V

    return float32_t3(hue, saturation, value); // H:0-360, S:0-1, V:0-1    
}

float32_t3 HSVToRGB(float32_t3 hsv) {   
    float32_t c = hsv.z * hsv.y; // Chroma
    float32_t x = c * (1.0f - abs(fmod(hsv.x / 60.0f, 2.0f) - 1.0f));
    float32_t m = hsv.z - c;

    float32_t3 rgb;

    if (hsv.x < 60.0f)
    {
        rgb = float32_t3(c, x, 0.0f);

    }
    else if (hsv.x < 120.0f)
    {
        rgb = float32_t3(x, c, 0.0f);

    }
    else if (hsv.x < 180.0f)
    {
        rgb = float32_t3(0.0f, c, x);

    }
    else if (hsv.x < 240.0f)
    {
        rgb = float32_t3(0.0f, x, c);

    }
    else if (hsv.x < 300.0f)
    {
        rgb = float32_t3(x, 0.0f, c);

    }
    else
    {
        rgb = float32_t3(c, 0.0f, x);
    }

    return rgb + m;
}
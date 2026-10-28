
float3 RGBToHSV(float3 rgb)
{
    float cmax = max(rgb.r, max(rgb.g, rgb.b));
    float cmin = min(rgb.r, min(rgb.g, rgb.b));
    float delta = cmax - cmin;

    float hue = 0.0f; // H
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

    float saturation = (cmax == 0.0f) ? 0.0f : (delta / cmax); // S
    float value = cmax; // V

    return float3(hue, saturation, value); // H:0-360, S:0-1, V:0-1    
}

float3 HSVToRGB(float3 hsv) {   
    float c = hsv.z * hsv.y; // Chroma
    float x = c * (1.0f - abs(fmod(hsv.x / 60.0f, 2.0f) - 1.0f));
    float m = hsv.z - c;

    float3 rgb;

    if (hsv.x < 60.0f)
    {
        rgb = float3(c, x, 0.0f);

    }
    else if (hsv.x < 120.0f)
    {
        rgb = float3(x, c, 0.0f);

    }
    else if (hsv.x < 180.0f)
    {
        rgb = float3(0.0f, c, x);

    }
    else if (hsv.x < 240.0f)
    {
        rgb = float3(0.0f, x, c);

    }
    else if (hsv.x < 300.0f)
    {
        rgb = float3(x, 0.0f, c);

    }
    else
    {
        rgb = float3(c, 0.0f, x);
    }

    return rgb + m;
}
#include "utils.hpp"
#include <string>
#include <hyprland/src/debug/log/Logger.hpp>

double activation(std::string function, double max, double value) {
    double result = 0;
    if (function == "linear") {

        result = value / max;

    } else if (function == "quadratic") {

        // (1 / m²) * x², is a quadratic function which will reach 1 at m
        result = (1.0 / (max * max)) * (value * value);
        result *= (value > 0 ? 1 : -1);

    } else if (function == "negative_quadratic") {

        float x = std::abs(value);
        // (-1 / m²) * (x - m)² + 1, is a quadratic function with the inverse curvature which will reach 1 at m
        result = (-1.0 / (max * max)) * ((x - max) * (x - max)) + 1;
        if (x > max) result = 1; // need to clamp manually, as the function would decrease again

        result *= (value > 0 ? 1 : -1);
    } else
        Log::logger->log(Log::WARN, "[dynamic-cursors] unknown air function specified");

    return std::clamp(result, -1.0, 1.0);
}

void SModeResult::clamp(double angle, double scale, double stretch) {
    if (std::abs(this->rotation) < angle)
        this->rotation = 0;

    if (std::abs(1 - this->scale) < scale)
        this->scale = 1;

    if (std::abs(1 - this->stretch.magnitude.x) < stretch && std::abs(1 - this->stretch.magnitude.x) < stretch)
        this->stretch.magnitude = Vector2D{1,1};
}

bool SModeResult::hasDifference(SModeResult* other, double angle, double scale, double stretch) {
    return
        std::abs(other->rotation - this->rotation) > angle ||
        std::abs(other->scale - this->scale) > scale ||
        std::abs(other->stretch.angle - this->stretch.angle) > angle ||
        std::abs(other->stretch.magnitude.x - this->stretch.magnitude.x) > stretch ||
        std::abs(other->stretch.magnitude.y - this->stretch.magnitude.y) > stretch;
}

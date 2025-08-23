#pragma once

#include "Node.h"

#include <cmath>

namespace model_utils
{
    double GetDistance(double x1, double y1, double x2, double y2)
    {
        return sqrt(pow (x2 - x1, 2) + pow (y2 - y1, 2));
    }
    double GetDistance(Node first, Node second)
    {
        return GetDistance(first.X, first.Y, second.X, second.Y);
    }
}

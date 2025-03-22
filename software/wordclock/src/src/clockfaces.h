#pragma once
#include "ClockFace.h"

auto position = ClockFace::LightSensorPosition::Bottom;

EnglishClockFace clockFaceEN(position);
FrenchClockFace clockFaceFR(position);
DutchClockFace clockFaceNL(position);
ItalianClockFace clockFaceIT(position);
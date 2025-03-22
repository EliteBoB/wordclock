#include <vector>
#include <NeoPixelBus.h>
#include <array>
#include <algorithm> // For std::min

const RgbColor WHITE = RgbColor(255, 255, 255);
const std::array<RgbColor, 16> COLOR_LOOKUP_TABLE PROGMEM = {
    WHITE,
    RgbColor(251, 243, 5),
    RgbColor(255, 100, 3),
    RgbColor(221, 9, 7),
    RgbColor(242, 8, 132),
    RgbColor(71, 0, 165),
    RgbColor(0, 0, 211),
    RgbColor(2, 171, 234),
    RgbColor(31, 183, 20),
    RgbColor(0, 100, 18),
    RgbColor(86, 44, 5),
    RgbColor(144, 113, 58),
    RgbColor(192, 192, 192),
    RgbColor(128, 128, 128),
    RgbColor(64, 64, 64),
    RgbColor(0, 0, 0)};

// Use a constant expression for the size of COLOR_LOOKUP_TABLE
constexpr size_t COLOR_LOOKUP_TABLE_SIZE = 16; // Fixed size of the array

namespace Palette
{
    const int CHARS_PER_COLOR = 1; // Use 2 for 256 colors palette.

    // Function to get the size of the palette
    constexpr size_t size() {
        return COLOR_LOOKUP_TABLE_SIZE; 
    }

    // Function to get color by index
    RgbColor getColor(size_t index) {
        if (index < size()) {
            return COLOR_LOOKUP_TABLE[index];
        }
        return RgbColor(0, 0, 0);  // Return black if index is out of bounds
    }

    static std::vector<RgbColor> stringToRgb(String payload, RgbColor defaultColor)
    {
        // Convert the Arduino String to std::string
        std::string str = payload.c_str();

        std::vector<RgbColor> colors;
        colors.reserve(132); // Pre-allocate space for efficiency

        // Use std::min for size calculation
        for (int i = 0; i < std::min(static_cast<int>(str.length()), 132 * CHARS_PER_COLOR); i += CHARS_PER_COLOR)
        {
            try
            {
                unsigned int colorIndex = std::stoul(str.substr(i, CHARS_PER_COLOR), nullptr, 16);

                // Check if the color index is within valid range
                if (colorIndex >= size()) {
                    colors.push_back(defaultColor);  // Invalid index, use default color
                } else {
                    colors.push_back(getColor(colorIndex)); // Valid index, use the corresponding color
                }
            }
            catch (std::invalid_argument const &ex)
            {
                colors.push_back(defaultColor); // Invalid argument, use default color
            }
        }

        return colors;
    }
}
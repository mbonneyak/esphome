// Datasheet https://wiki.dfrobot.com/A01NYUB%20Waterproof%20Ultrasonic%20Sensor%20SKU:%20SEN0313
// Manual https://cdn.sparkfun.com/assets/1/4/2/1/9/TFmini_Plus_A02_Product_Manual_EN.pdf

// Byte0 0x59, frame header, same for each frame 
// Byte1 0x59, frame header, same for each frame 
// Byte2 Dist_L distance value lower by 8 bits 
// Byte3 Dist_L distance value higher by 8 bits 
// Byte4 Strength_L low 8 bits 
// Byte5 Strength_L high 8 bits 
// Byte6 Temp_L low 8 bits (suit for version later than V1.3.0) 
// Byte7 Temp_H high 8 bits (suit for version later than V1.3.0) 
// Byte8 Checksum is the lower 8 bits of the cumulative sum of the numbers of the first 8 bytes.

#include "tfminiplus.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"

namespace esphome {
namespace tfminiplus {

static const char *const TAG = "tfminiplus.sensor";
static const uint8_t MAX_DATA_LENGTH_BYTES = 8;

void TfminiplusComponent::loop() {
  uint8_t data;
  while (this->available() > 0) {
    if (this->read_byte(&data)) {
      buffer_.push_back(data);
      this->check_buffer_();
    }
  }
}

void TfminiplusComponent::check_buffer_() {
  if (this->buffer_.size() >= MAX_DATA_LENGTH_BYTES) {
    //added line below for debug
    //ESP_LOGV(TAG, "Max Data Length Reached", 1, 2);
    size_t i;
    for (i = 0; i < this->buffer_.size(); i++) {
      // Look for the first packet
      if (this->buffer_[i] == 0x59) {
        //added line below for debug
        //ESP_LOGV(TAG, "Packet Complete", distance, meters);
        if (i + 1 + 8 < this->buffer_.size()) {  // Packet is not complete
          return;                                // Wait for completion
        }

        uint8_t checksum = (this->buffer_[i] + this->buffer_[i + 1] + this->buffer_[i + 2] + this->buffer_[i + 3] + this->buffer_[i + 4] + this->buffer_[i + 5] + this->buffer_[i + 6] + this->buffer_[i + 8]) & 0x59;
        if (this->buffer_[i + 9] == checksum) {
          float distance = (this->buffer_[i + 2] << 8) + this->buffer_[i + 3];
          //if (distance > 280) {
          if (distance > 0) {
            float meters = distance / 1000.0;
            ESP_LOGV(TAG, "Distance from sensor: %f mm, %f m", distance, meters);
            this->publish_state(meters);
          } else {
            ESP_LOGW(TAG, "Invalid data read from sensor: %s", format_hex_pretty(this->buffer_).c_str());
          }
        }
        break;
      }
    }
    this->buffer_.clear();
  }
}

void TfminiplusComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "TFminiPlus Sensor:");
  LOG_SENSOR("  ", "Distance", this);
}

}  // namespace tfminiplus
}  // namespace esphome

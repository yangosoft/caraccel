#include "gps.hpp"
#include "esp_log.h"
#include <sstream>
#include <string>

extern "C" void parse_vtg(const char *cstr_sentence, gps_data_t *gps_data);
void parse_rmc(const char *cstr_sentence, gps_data_t *gps_data);
void parse_gll(const char *cstr_sentence, gps_data_t *gps_data);

extern "C" void parse_nmea_sentence(const char *cstr_sentence, gps_data_t *gps_data)
{

    const std::string sentence(cstr_sentence);

    if (sentence.length() < 16)
    {
        ESP_LOGI("GPS", "SENTENCE NOT VALID");
        return;
    }

    ESP_LOGI("GPS", "Parsing sentence: '%s'", sentence.c_str());

    std::string prefix = sentence.substr(3, 3);

    if (prefix == "VTG")
    {
        // ESP_LOGI("GPS", "VTG");
        parse_vtg(cstr_sentence, gps_data);
    }
    else if (prefix == "GLL")
    {
        // ESP_LOGI("GPS", "GLL");
        parse_gll(cstr_sentence, gps_data);
    }
    else if (prefix == "RMC")
    {
        // ESP_LOGI("GPS", "RMC");
        parse_rmc(cstr_sentence, gps_data);
    }
    else
    {
        ESP_LOGI("GPS", "Sentence cannot be parsed: '%s'", sentence.c_str());
        // Serial.print(sentence.c_str());
        // Serial.println("'");
    }
}

extern "C" void parse_vtg(const char *cstr_sentence, gps_data_t *gps_data)
{
    std::stringstream test(cstr_sentence);
    std::string segment;

    int i = 0;

    double tmp_speed_kph = 0;

    while (std::getline(test, segment, ','))
    {
        if ((i == 7) && (!segment.empty()))
        {
            // Serial.print("Speed ");
            // Serial.println(segment.c_str());
            tmp_speed_kph = std::stod(segment);
        }
        else if ((i == 9) && (!segment.empty()))
        {
            gps_data->data_validity = segment[0];
            if (((gps_data->data_validity == 'A') || (gps_data->data_validity == 'D')) &&
                (tmp_speed_kph > 0) && (tmp_speed_kph < 360))
            {

                gps_data->speed_kph = tmp_speed_kph;
            }
            else
            {
                // Serial.print("DATA no valid: ");
                // Serial.println(gps_data->data_validity.c_str());
                gps_data->speed_kph = 0;
            }
            return;
        }
        ++i;
    }
}

void parse_gll(const char *cstr_sentence, gps_data_t *gps_data)
{

    std::stringstream test(cstr_sentence);
    std::string segment;
    std::string lat, lon;

    int i = 0;

    while (std::getline(test, segment, ','))
    {
        if ((i == 5) && (!segment.empty()))
        {
            strncpy(gps_data->timestr, segment.c_str(), sizeof(gps_data->timestr) - 1);
            gps_data->timestr[sizeof(gps_data->timestr) - 1] = '\0';
        }

        if ((i == 6) && (!segment.empty()))
        {
            gps_data->data_validity = segment[0];

            strcpy(gps_data->lat, lat.c_str());
            strcpy(gps_data->lon, lon.c_str());
            return;
        }

        if ((i == 1) && (!segment.empty()))
        {
            lat = segment;
        }

        if ((i == 2) && (!segment.empty()))
        {
            lat += segment;
        }

        if ((i == 3) && (!segment.empty()))
        {
            lon = segment;
        }

        if ((i == 4) && (!segment.empty()))
        {
            lon += segment;
        }
        ++i;
    }
}

void parse_rmc(const char *cstr_sentence, gps_data_t *gps_data)
{

    std::stringstream test(cstr_sentence);
    std::string segment;
    std::string lat, lon;

    int i = 0;

    // Serial.println("PARSING RMC");
    while (std::getline(test, segment, ','))
    {
        if ((i == 1) && (!segment.empty()))
        {
            gps_data->timestr[0] = '\0';
            strncpy(gps_data->timestr, segment.c_str(), sizeof(gps_data->timestr) - 1);
            gps_data->timestr[sizeof(gps_data->timestr) - 1] = '\0';
        }

        if ((i == 2) && (!segment.empty()))
        {
            gps_data->data_validity = segment[0];
        }

        if ((i == 3) && (!segment.empty()))
        {
            lat = segment;
        }

        if ((i == 4) && (!segment.empty()))
        {
            lat += segment;
        }

        if ((i == 5) && (!segment.empty()))
        {
            lon = segment;
        }

        if ((i == 6) && (!segment.empty()))
        {
            lon += segment;
        }

        if ((i == 7) && (!segment.empty()))
        {
            gps_data->speed_knots = std::stod(segment);
            gps_data->speed_kph = gps_data->speed_knots * 1.852;
            gps_data->lat[0] = '\0';
            gps_data->lon[0] = '\0';
            strcpy(gps_data->lat, lat.c_str());
            strcpy(gps_data->lon, lon.c_str());
            // Serial.println("PARSED OK");
            return;
        }
        ++i;
    }
}
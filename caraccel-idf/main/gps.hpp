#ifdef __cplusplus
extern "C"
{
#endif

    struct gps_data_t
    {
        char data_validity;
        char lat[16];
        char lon[16];
        double speed_kph;
        double speed_knots;
        char timestr[11];
        // double altitude;
    };

    void parse_nmea_sentence(const char *cstr_sentence, struct gps_data_t *gps_data);

#ifdef __cplusplus
}
#endif
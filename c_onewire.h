
struct sOneWireDevice {
    char *adress;
    char *type;
    float value;
    struct sOneWireDevice *next;
};

unsigned get_file_size(const char * file_name);
struct sOneWireDevice *list_devices();
struct sOneWireDevice *updateValue(struct sOneWireDevice *device);

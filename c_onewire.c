#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#include "c_onewire.h"
#include "log.h"

static char oneWirePath[] = "/sys/bus/w1/devices";
extern double power;
extern int elapsedTicks;

unsigned get_file_size(const char * file_name) {
    struct stat sb;

    if (stat(file_name, & sb) != 0) {
        sprintf(log_buf, "'stat' failed for '%s': %s",
                file_name, strerror(errno));
        log_entry(log_buf, LOG_EXIT);
    }
    return sb.st_size;
}

struct sOneWireDevice *list_devices() {
    DIR *pDirHandle;
    struct dirent *pDir;
    struct sOneWireDevice *sRetVal = NULL;
    struct sOneWireDevice *sDevice = NULL;


    pDirHandle = opendir(oneWirePath);

    if (pDirHandle == NULL) {
        sprintf(log_buf, "Error opening \"%s\"", oneWirePath);
        log_entry(log_buf, LOG_EXIT);
    }

    while ((pDir = readdir(pDirHandle))) {

        // for each of the directories in the list, we need to check to see if each
        // is a valid sensor.

        // Ignore the directory 'w1_bus_master1'
        // Ignore the directory '.'
        // Ignore the directory '..'
        if (pDir->d_name[0] == '.') {
            continue;
        }

        if (pDir->d_name[0] == 'w') {
            continue;
        }

        if (sRetVal == NULL) {
            sRetVal = malloc(sizeof (struct sOneWireDevice));
            sRetVal->next = NULL;
            sDevice = sRetVal;
        } else {
            sDevice = sRetVal;
            while (sDevice->next != NULL) {
                sDevice = sDevice->next;
            }
            sDevice->next = malloc(sizeof (struct sOneWireDevice));
            sDevice = sDevice->next;
            sDevice->next = NULL;
        }

        sDevice->adress = strdup(pDir->d_name);

        switch (pDir->d_name[0]) {
            case '1':
                if (pDir->d_name[1] == '0') {
                    sDevice->type = strdup("DS18S20");
                } else {
                    sDevice->type = strdup("UNKNOWN");
                }
                break;
            case '2':
                if (pDir->d_name[1] == '8') {
                    sDevice->type = strdup("DS18B20");
                } else {
                    sDevice->type = strdup("UNKNOWN");
                }
                break;
            default:
                sDevice->type = strdup("UNKNOWN");
                break;
        }

    }
    closedir(pDirHandle);
    sDevice = sRetVal;
    while (sDevice->next != NULL) {
        sDevice = sDevice->next;
    }
    sDevice->next = malloc(sizeof (struct sOneWireDevice));
    sDevice = sDevice->next;
    sDevice->adress = strdup("000000000000001");
    sDevice->type = strdup("WATT");
    
    sDevice->next = malloc(sizeof (struct sOneWireDevice));
    sDevice = sDevice->next;
    sDevice->adress = strdup("000000000000002");
    sDevice->type = strdup("KWH");

    return sRetVal;
}

struct sOneWireDevice *updateValue(struct sOneWireDevice *device) {
    unsigned s;
    char *contents;
    FILE *pFile;
    char deviceFile[200];
    int i;
    char *buf;
    int valid;
    
    if(strcmp(device->type, "WATT") == 0){
        device->value = (power / elapsedTicks);
        return device;
    }

    if(strcmp(device->type, "KWH") == 0){
        device->value = elapsedTicks;
        return device;
    }

    sprintf(deviceFile, "%s/%s/w1_slave", oneWirePath, device->adress);

    s = get_file_size(deviceFile);

    pFile = fopen(deviceFile, "rb");

    if (pFile != NULL) {
        contents = malloc(s + 1);
        fread(contents, sizeof (unsigned char), s, pFile);
        buf = strdup(strstr(contents, "crc=") + 7);
        buf = strtok(buf, "\r\n");
        fclose(pFile);

        //CRC Check
        valid = (strcmp(buf, "YES") == 0);

        free(buf);
        buf = strdup(strstr(contents, "t=") + 2);
        sscanf(buf, "%i", &i);
        device->value = i / 1000.0;

        if (!valid) {
            sprintf(log_buf, "CRC Error on device %s, value=%4.2f", 
device->adress, device->value);
            log_entry(log_buf, LOG_NO_EXIT);
            device = NULL;
        }
        free(buf);
        free(contents);
        return device;
    }
    return NULL;

}



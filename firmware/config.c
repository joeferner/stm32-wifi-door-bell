
#include "config.h"
#include <stdio.h>
#include <string.h>
#include "stm32lib/cc3000-host-driver/wlan.h"

#define INI_SECTION_UNKNOWN 0
#define INI_SECTION_WIFI    1

Config config;

int _config_stringToWlanSecurity(const char* security);

BOOL config_read() {
  int iniSection = INI_SECTION_UNKNOWN;
  char buffer[100];
  char* name;
  char* value;

  FILE* pFile = fopen("/mnt/sdcard/config.ini", "r");
  if (pFile == NULL) {
    printf("Failed to open config.ini\n");
    return FALSE;
  }

  printf("reading config\n");
  while (fgets(buffer, sizeof(buffer), pFile) != NULL) {
    strTrim(buffer);
    printf("config: %s\n", buffer);
    name = NULL;
    if ((value = strchr(buffer, '='))) {
      name = buffer;
      *value = '\0';
      value++;
    }

    if (strncmp(buffer, "[wifi]", strlen("[wifi]")) == 0) {
      iniSection = INI_SECTION_WIFI;
    } else if (name && value) {
      if (iniSection == INI_SECTION_WIFI) {
        if (strcmp(name, "ssid") == 0) {
          strcpy(config.wifi.ssid, value);
        } else if (strcmp(name, "password") == 0) {
          strcpy(config.wifi.password, value);
        } else if (strcmp(name, "security") == 0) {
          config.wifi.security = _config_stringToWlanSecurity(value);
          if (config.wifi.security == -1) {
            printf("invalid wifi security %s\n", value);
            goto fail;
          }
        } else {
          printf("Unknown name %s in section wifi\n", name);
        }
      } else {
        printf("Unknown name %s in section %d\n", name, iniSection);
      }
    }
  }
  fclose(pFile);
  return TRUE;

fail:
  fclose(pFile);
  return FALSE;
}

int _config_stringToWlanSecurity(const char* security) {
  if (strcasecmp(security, "WLAN_SEC_UNSEC") == 0) {
    return WLAN_SEC_UNSEC;
  }
  if (strcasecmp(security, "WLAN_SEC_WEP") == 0) {
    return WLAN_SEC_WEP;
  }
  if (strcasecmp(security, "WLAN_SEC_WPA") == 0) {
    return WLAN_SEC_WPA;
  }
  if (strcasecmp(security, "WLAN_SEC_WPA2") == 0) {
    return WLAN_SEC_WPA2;
  }
  return -1;
}

#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define VENDOR_BROTHER 0x04f9
#define PRODUCT_DCP7010 0x0182

#define SCANNER_INTERFACE 1

#define TIMEOUT 30000


#define BREQ_TYPE 0xC0
#define BREQ_GET_OPEN 0x01
#define BREQ_GET_CLOSE 0x02

#define BREQ_GET_LENGTH 5

#define BDESC_TYPE 0x10

#define BCOMMAND_RETURN 0x8000

#define BCOMMAND_SCANNER 0x02

#define PAGE_WIDTH 816
#define PAGE_HEIGHT 1376


#define MFCMD_QUERYDEVINFO   "\x1BQ\n\x80"


typedef char	CHAR;
typedef short SHORT;
typedef long	LONG;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned char       *LPBYTE;
typedef unsigned short      WORD;
typedef unsigned long	DWORD;
typedef float             FLOAT;
typedef int               INT;
typedef unsigned int      UINT;
typedef unsigned int      *PUINT;
typedef char	   *LPSTR;
//typedef char	   *PSTR;
typedef char	   *LPTSTR;
//typedef const char *PCSTR;
//typedef const char *LPCSTR;
typedef	const char *LPCTSTR;




typedef struct tagMFCDEVICEHEAD {
    WORD  wDeviceInfoID;		// �ǥХ��������ID
    BYTE  nInfoSize;		// �ǥХ�������Υ�������ID�ϥ������˴ޤޤʤ���
    BYTE  nProtcolType; 		// DS<->MFC�֤Υץ�ȥ������
    // 00h=��1999ǯ��ǥ�, 01h=2000ǯ��ǥ�
} MFCDEVICEHEAD, *LPMFCDEVICEHEAD;

typedef union tagMFCCOLORTYPE {
    struct {
        BYTE  bBlackWhite:1;	// ���͡������
        BYTE  bErrorDiffusion:1;// ���Ȼ�
        BYTE  bTrueGray:1;	// ���졼��������
        BYTE  b256Color:1;	// 256�����顼
        BYTE  b24BitColor:1;	// 24bit���顼
        BYTE  b256IndexColor:1;	// 256�����顼��MFC����ѥ�åȥơ��֥������
    } bit;
    BYTE val;
} MFCCOLORTYPE, *LPMFCCOLORTYPE;

typedef struct tagMFCDEVICEINFO {
    BYTE          nVideoSignalType; // �ӥǥ��ο������
    //   00h=Reserve, FFh=�ӥǥ�̵��, 01h=NTSC, 02h=PAL
    MFCCOLORTYPE  nColorType;	// ������ʤ��б����顼������
    //   00h=Reserve, MSB|0:0:256ix:24c:256c:TG:ED:BW|LSB
    WORD          nVideoNtscSignal; // NTSC���浬��
    //   0=Reserve 1:B,2:G,3:H,4:I,5:D,6:K,7:K1,8:L,9:M,10:N
    WORD          nVideoPalSignal;	// PAL���浬��
    //   0=Reserve 1:B,2:G,3:H,4:I,5:D,6:K,7:K1,8:L,9:M,10:N
    WORD          nVideoSecamSignal;// SECAM���浬��
    //   0=Reserve 1:B,2:G,3:H,4:I,5:D,6:K,7:K1,8:L,9:M,10:N
    BYTE          nHardwareType;	// ���Ĥμ���
    //   00h=Reserve, 01h=NTSC���� 02h=NTSC/Lexmark���� 81h=PAL����
    BYTE          nHardwareVersion; // ���ĤΥС������
    //   00h=Reserve, 01h��=�С�������ֹ�
    BYTE          nMainScanDpi;	// ������ʼ����������٤�ǽ��
    //   00h=Reserve, 01h=200dpi, 02h=200,300dpi, 03h=100,200,300dpi
    BYTE          nPaperSizeMax;	// ��������б��ѻ極����
    //   00h=Reserve, 01h=A4, 02h=B4
} MFCDEVICEINFO, *LPMFCDEVICEINFO;


struct program_arguments {
    int resolution;
    char* mode;
    char* file;
};

struct found_scanner {
    libusb_device *dev;
    uint8_t serial_number_index;
};

struct found_scanner * print_devs(libusb_device **devs)
{
    libusb_device *dev;
    int i = 0, j = 0;
    uint8_t path[8];
    int ret = 0;
    char serial_number[32];
    int found_counter = 0;
    struct found_scanner *found = NULL;

    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < LIBUSB_SUCCESS) {
            fprintf(stderr, "failed to get device descriptor");
            return NULL;
        }

        if (desc.idVendor == VENDOR_BROTHER && desc.idProduct == PRODUCT_DCP7010) {

            found_counter++;

            printf("%04x:%04x (bus %d, device %d), serial number index %d\n",
                   desc.idVendor,
                   desc.idProduct,
                   libusb_get_bus_number(dev),
                   libusb_get_device_address(dev),
                   desc.iSerialNumber);

            if (found == NULL) {
                found = malloc(sizeof(struct found_scanner));
            }
            found->dev = dev;
            found->serial_number_index = desc.iSerialNumber;
        }
    }

    if (found_counter > 1) {
        fprintf(stderr, "Too many devices found - this is not supported\n");
        free(found);
    } else if (found_counter == 1) {
        return found;
    }

    return NULL;
}

int print_serial_number(libusb_device_handle *handle, uint8_t serial_number_index) {
    char serial_number[32];
    int ret;

    ret = libusb_get_string_descriptor_ascii(
            handle,
            serial_number_index,
            (unsigned char *) serial_number,
            sizeof(serial_number));
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot read serial number %s\n", libusb_error_name(ret));
        return ret;
    }

    printf("Serial number is %s\n", serial_number);

    return 0;
}

int close_scanner( libusb_device_handle *handle )
{
    int ret;
    unsigned char data[BREQ_GET_LENGTH];

    printf("Closing scanner\n");

    ret = libusb_control_transfer(handle,
                                  LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
                                  BREQ_GET_CLOSE,
                                  BCOMMAND_SCANNER,
                                  0,
                                  data,
                                  BREQ_GET_LENGTH,
                                  TIMEOUT);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot send close command %s\n", libusb_error_name(ret));

        return -1;
    }

    return 0;
}

#define CONFIGURATION_DATA_SIZE 255

int configure_scanner(libusb_device_handle *pHandle, struct program_arguments *program_arguments) {
    char *text_config = calloc(CONFIGURATION_DATA_SIZE, sizeof(char));
    snprintf(
            text_config,
            CONFIGURATION_DATA_SIZE,
            "R=%d,%d\nM=%s\nC=NONE\nB=50\nN=50\nU=OFF\nP=OFF\nA=0,0,%d,%d\n",
             program_arguments->resolution,
             program_arguments->resolution,
             program_arguments->mode,
             PAGE_WIDTH * program_arguments->resolution / 100,
             PAGE_HEIGHT * program_arguments->resolution / 100);

    printf("Sending configuration config: %s\n", text_config);

    char *command = calloc(CONFIGURATION_DATA_SIZE, sizeof(char));
    int length = snprintf(command, CONFIGURATION_DATA_SIZE, "\x1bX\n%s\x80", text_config);

    int transferred;
    int ret = libusb_bulk_transfer(pHandle,
                                   LIBUSB_ENDPOINT_OUT | 3,
                                   command, length, &transferred, TIMEOUT);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot send scan command %s\n", libusb_error_name(ret));
    }

    printf("Sent %d scan command bytes\n", transferred);

    free(command);
    free(text_config);

    return 0;
}

int bulk_read(libusb_device_handle *handle, unsigned char *buf, int length) {
    int ret, n;
    ret = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_IN | 4, buf, length, &n, TIMEOUT);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot read from scanner %s\n", libusb_error_name(ret));

        return -1;
    }
    return n;
}

#define READ_BUFFER_SIZE 0x3000

#define SLEEP_TIME_USEC (200*1000)

#define SLEEPS_COUNT 40
#define SLEEPS_COUNT_TIMEOUT 100

int read_page(libusb_device_handle *handle, struct program_arguments *program_arguments) {
    FILE *fp = NULL;
    unsigned char buffer[READ_BUFFER_SIZE];

    int sleeps_count = 0;
    while (1) {
        int num_bytes = bulk_read(handle, (unsigned char *) &buffer, READ_BUFFER_SIZE);
        if (num_bytes > 0) {
            sleeps_count = 0;
        }

        if (num_bytes < 0) {
            break;
        } else if (num_bytes > 2) {
            printf("Read %d bytes from scanner\n", num_bytes);

            if (fp == NULL) {
                char *fname = program_arguments->file;
                printf("Opening '%s'\n", fname);
                fp = fopen(fname, "wb");
                if (fp == NULL) {
                    fprintf(stderr, "Cannot open file '%s' error=%s\n", fname, strerror(errno));
                    return 3;
                }
            }

            fwrite(buffer, 1, num_bytes, fp);
        } else if (num_bytes == 0 && sleeps_count < SLEEPS_COUNT) {
            sleeps_count++;
            printf("sleeping\n");
            usleep(SLEEP_TIME_USEC);
        } else if (num_bytes == 2 && buffer[0] == 0xc2 && buffer[1] == 0x00) {
            fprintf(stderr, "ERROR: nothing to scan\n");
            break;
        } else if (num_bytes == 2 && buffer[0] == 0xc3 && buffer[1] == 0x00) {
            fprintf(stderr, "ERROR: Paper jam\n");
            break;
        } else if (num_bytes == 1 && buffer[0] == 0x80) {
            printf("No more pages\n");
            break;
        } else if (num_bytes == 1 && buffer[0] == 0x81 && sleeps_count >= SLEEPS_COUNT) {
            fprintf(stderr, "Feeding in another page");
            if (sleeps_count >= SLEEPS_COUNT_TIMEOUT) {
                fprintf(stderr, " (timeout)");
            }
            fprintf(stderr, "\n");

            sleeps_count = 0;
        } else if (num_bytes == 1 && buffer[0] == 0xc3) {
            fprintf(stderr, "ERROR: Paper jam\n");
            break;
        } else if (num_bytes == 1 && buffer[0] == 0xc4) {
            fprintf(stderr, "Scan aborted\n");
            break;
        } else {
            fprintf(stderr, "Received %d bytes, ", num_bytes);
            fprintf(stderr, "Received unknown data %02x", buffer[0]);
            if (num_bytes == 2) {
                fprintf(stderr, " %02x", buffer[1]);
            }
            fprintf(stderr, "\n");
            break;
        }

    }

    if (fp != NULL) {
        fclose(fp);
    }

    return 0;
}

int init_scanner(libusb_device_handle *handle) {
    int ret;
    char data[BREQ_GET_LENGTH];

    ret = libusb_control_transfer(
            handle,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
            BREQ_GET_OPEN,
            BCOMMAND_SCANNER,
            0,
            data,
            BREQ_GET_LENGTH,
            TIMEOUT
    );
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot read get open %s\n", libusb_error_name(ret));
        return ret;
    }

    // /* returns 05 10 01 02 00 */
    printf("data: ");
    for (int i = 0; i < BREQ_GET_LENGTH; i++) {
        printf("%02X", (unsigned char) data[i]);
    }
    printf("\n");

    // check the size of discriptor
    int nValue = (int) data[0];
    if (nValue != BREQ_GET_LENGTH) {
        fprintf(stderr, "Control command status is bad BREQ_GET_LENGTH\n");
        return -60;
    }

    // check the type of discriptor
    nValue = (int) data[1];
    if (nValue != BDESC_TYPE) {
        fprintf(stderr, "Control command status is bad BDESC_TYPE\n");
        return -60;
    }

    // check the command ID
    nValue = (int) data[2];
    if (nValue != BREQ_GET_OPEN) {
        fprintf(stderr, "Control command status is bad BREQ_GET_OPEN\n");
        return -60;
    }

    // check the command parameters
    nValue = (int) *((WORD *) &data[3]);
    if (nValue & BCOMMAND_RETURN) {
        fprintf(stderr, "Control command status is bad BCOMMAND_RETURN\n");
        return -60;
    }

    if (nValue != BCOMMAND_SCANNER) {
        fprintf(stderr, "Control command status is bad BCOMMAND_SCANNER\n");
        return -60;
    }

    return 0;
}

#define DRAIN_BUFFER 32000

int drain_invalid_data_on_scanner(libusb_device_handle *handle) {
    int ret;
    int transferred;

    // reset scanner
    unsigned char *usb_drain_buffer = malloc(DRAIN_BUFFER);
    ret = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_IN | 4, usb_drain_buffer, DRAIN_BUFFER, &transferred, TIMEOUT);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "USB drain failed %s\n", libusb_error_name(ret));

        free(usb_drain_buffer);

        return ret;
    }

    printf("Drained %d bytes\n", transferred);

    free(usb_drain_buffer);

    return 0;
}

int scan(struct program_arguments *program_arguments) {
    libusb_device **devs;
    struct found_scanner *found;
    int transferred;

    ssize_t cnt;

    cnt = libusb_get_device_list(NULL, &devs);
    if (cnt < LIBUSB_SUCCESS) {
        return -1;
    }

    found = print_devs(devs);

    libusb_free_device_list(devs, 1);

    if (found == NULL) {
        return 1;
    }

    libusb_device_handle *handle;
    int ret;

    ret = libusb_open(found->dev, &handle);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot open device %s\n", libusb_error_name(ret));
        free(found);
        return ret;
    }

    print_serial_number(handle, found->serial_number_index);

    ret = libusb_set_configuration(handle, SCANNER_INTERFACE);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot set configuration %s", libusb_error_name(ret));
        free(found);
        libusb_close(handle);

        return ret;
    }

    ret = libusb_claim_interface(handle, SCANNER_INTERFACE);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot claim scanner interface %s\n", libusb_error_name(ret));
        free(found);
        libusb_close(handle);

        return ret;
    }

    ret = libusb_set_interface_alt_setting(handle, SCANNER_INTERFACE, 0);
    if (ret < LIBUSB_SUCCESS) {
        fprintf(stderr, "Cannot set alternate settings %s\n", libusb_error_name(ret));
        free(found);
        libusb_release_interface(handle, SCANNER_INTERFACE);
        libusb_close(handle);

        return ret;

    }
    // libusb_clear_halt(handle, )

    // control_in_vendor_device(1, 2, 0, 5); /* returns 05 10 01 02 00 */
    ret = init_scanner(handle);
    if (ret < 0) {
        libusb_release_interface(handle, SCANNER_INTERFACE);
        free(found);
        libusb_close(handle);

        return -60;
    }

    ret = drain_invalid_data_on_scanner(handle);
    if (ret < 0) {
        close_scanner(handle);

        libusb_release_interface(handle, SCANNER_INTERFACE);
        free(found);
        libusb_close(handle);

        return ret;
    }

//    // Q_Command
//    ret = libusb_bulk_transfer(handle,
//            LIBUSB_ENDPOINT_OUT | 3,
//            MFCMD_QUERYDEVINFO, sizeof(MFCMD_QUERYDEVINFO), &transferred, TIMEOUT);
//    if (ret < LIBUSB_SUCCESS) {
//        fprintf(stderr, "Cannot write Q Command %s\n", libusb_error_name(ret));
//        free(usb_drain_buffer);
//        close_scanner(handle);
//
//        libusb_release_interface(handle, 0x01);
//        free(found);
//        libusb_close(handle);
//
//        return ret;
//    }
//
//    printf("Transferred %d bytes\n", transferred);
//
//    sleep(5);
//
//    // read Q_result
//    int qCommandReadSize = sizeof(MFCDEVICEHEAD) + sizeof(MFCDEVICEINFO);
//    unsigned char *qCommandReadBuffer = malloc(qCommandReadSize);
//    if (qCommandReadBuffer == NULL) {
//        free(usb_drain_buffer);
//        close_scanner(handle);
//
//        libusb_release_interface(handle, 0x01);
//        free(found);
//        libusb_close(handle);
//
//        return -1;
//    }
//
//    ret = libusb_bulk_transfer(handle, LIBUSB_ENDPOINT_IN | 4, qCommandReadBuffer, qCommandReadSize, &transferred, TIMEOUT);
//    if (ret < LIBUSB_SUCCESS) {
//        fprintf(stderr, "Cannot read Q Command %s\n", libusb_error_name(ret));
//
//        free(usb_drain_buffer);
//        free(qCommandReadBuffer);
//        close_scanner(handle);
//
//        libusb_release_interface(handle, 0x01);
//        free(found);
//        libusb_close(handle);
//
//        return ret;
//    }
//
//
//
//    printf("Transferred %d\n", transferred);
//
//    MFCDEVICEHEAD mfc_device_head;
//    MFCDEVICEINFO mfc_device_info;
//
//    mfc_device_head.wDeviceInfoID = (*(WORD *) qCommandReadBuffer);
//    mfc_device_head.nInfoSize = *(BYTE *)(qCommandReadBuffer+2);
//    mfc_device_head.nProtcolType = *(BYTE *)(qCommandReadBuffer+3);
//
//    memset(&mfc_device_info, 0, sizeof(MFCDEVICEINFO));
//
//    mfc_device_info.nColorType.val = *(BYTE *)(qCommandReadBuffer+5);
//    mfc_device_info.nHardwareVersion = *(BYTE *)(qCommandReadBuffer+13);
//    mfc_device_info.nMainScanDpi = *(BYTE *)(qCommandReadBuffer+14);
//    mfc_device_info.nPaperSizeMax = *(BYTE *)(qCommandReadBuffer+15);
//
//    free(qCommandReadBuffer);

    configure_scanner(handle, program_arguments);

    read_page(handle, program_arguments);

    close_scanner(handle);

    libusb_release_interface(handle, SCANNER_INTERFACE);
    free(found);
    libusb_close(handle);

    return ret;
}

char *parse_mode(char mode) {
    char *rmode;
    switch (mode) {
        case 'c': rmode = "CGRAY";  break;
        case 'g': rmode = "GRAY64"; break;
        case 't': rmode = "TEXT";   break;
        default:
            fprintf(stderr, "ERROR: unrecognised mode, "
                            "should be one of [cgt]\n");
            exit(2);
    }

    return rmode;
}

int parse_arguments(int argc, char **argv, struct program_arguments *args) {
    int resolution;
    int c;

    args->resolution = 100;
    args->mode = "CGRAY";
    args->file = NULL;

    while ((c = getopt(argc, argv, "r:m:f:")) != -1) {
        switch (c) {
            case 'f':
                args->file = optarg;
                break;
            case 'r':
                resolution = atoi(optarg);
                if (resolution < 100 || resolution > 600 || resolution % 100 != 0) {
                    fprintf(stderr, "Resolution must be between 100 and 600, and be module 100");
                    exit(2);
                }
                args->resolution = resolution;
                break;
            case 'm':
                args->mode = parse_mode(optarg[0]);
                break;
        }
    }

    if (args->file == NULL) {
        fprintf(stderr, "File not specified\n");
        exit(2);
    }

    printf("Arguments: file=%s resolution=%d mode=%s\n", args->file, args->resolution, args->mode);

    return 0;
}

int main(int argc, char **argv) {
    struct program_arguments program_arguments;

    printf("Hello, DCP-7020L!\n");

    parse_arguments(argc, argv, &program_arguments);

    int r = libusb_init(NULL);
    if (r < LIBUSB_SUCCESS) {
        fprintf(stderr, "USB init failed %s\n", libusb_error_name(r));
        return 127;
    }

    printf("lib usb inited\n");

    r = scan(&program_arguments);

    libusb_exit(NULL);

    return r;
}

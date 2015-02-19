/* by xerpi (c) 2013 */

#include "nunchuk.h"


static const char* nunchuk_adapter[] = {
	"/dev/i2c-1",
	"/dev/i2c-0",
	NULL
};

static uint8_t nunchuk_data_buffer[6];
static struct i2c_rdwr_ioctl_data nunchuk_ioctl_data;
static int nunchuk_fd = -1;
static int nunchuk_initiated = 0;
static int nunchuk_request_freq = 100;
static int nunchuk_request_period = 0;
static int nunchuk_run_thread     = 0;
static nuncuk_read_callback nuncuk_read_callback_func = NULL;
static pthread_t nunchuk_thread;

#define nunchuk_return_not_initiated() \
	if(!nunchuk_initiated) {    \
		return -2;             \
	}	

static int nunchuk_calculate_request_period()
{
	nunchuk_request_period = (int)((1000.0f * 1000)/(float)nunchuk_request_freq);
}

static int nunchuk_request_data()
{
	nunchuk_return_not_initiated();
	if(i2c_smbus_write_byte(nunchuk_fd, 0x00) < 0) {
		printf("error requesting data\n");
		return -1;
	}
	return 1;
}

static void nunchuk_try_callback(int read_succes)
{
	if(nuncuk_read_callback_func) {
		nuncuk_read_callback_func(read_succes);
	}	
}

static int nunchuk_parse_data(struct nunchuk* n)
{
	n->X = nunchuk_data_buffer[0];
	n->Y = nunchuk_data_buffer[1];
	n->Z = (~nunchuk_data_buffer[5]) & 0b1;
	n->C = ((~nunchuk_data_buffer[5])>>1) & 0b1;
	n->aX = (((nunchuk_data_buffer[5]>>2) & 0b11) | ((int)nunchuk_data_buffer[2])<<2);
	n->aY = (((nunchuk_data_buffer[5]>>4) & 0b11) | ((int)nunchuk_data_buffer[3])<<2);
	n->aZ = (((nunchuk_data_buffer[5]>>6) & 0b11) | ((int)nunchuk_data_buffer[4])<<2);
}

void* nunchuk_loop(void* n)
{
	while(nunchuk_run_thread) {
		if(nunchuk_read_data(n) > 0) {
			usleep(nunchuk_request_period);
		} else {
			while((nunchuk_init_nunchuk() < 0) && nunchuk_run_thread) {
				usleep(NUNCHUK_REINIT_DELAY);
			}
		}
	}
	return n;
}

int nunchuk_init_nunchuk()
{
	nunchuk_return_not_initiated();

	if( (i2c_smbus_write_byte_data(nunchuk_fd, 0xF0, 0x55) < 0) ||
		(i2c_smbus_write_byte_data(nunchuk_fd, 0xFB, 0x00) < 0))
	{
		printf("error initializing nunchuk...reconnect it again please\n");
		return -1;	
	}
	return 1;
}

int nunchuk_init(struct nunchuk* n)
{
	if(nunchuk_initiated) {
		return 2;
	}
	
	nunchuk_calculate_request_period();
	
	int i = 0;
	for(; nunchuk_adapter[i]; i++) {
		printf("Trying to open %s...", nunchuk_adapter[i]);
		nunchuk_fd = open(nunchuk_adapter[i], O_RDWR);
		if(nunchuk_fd < 0) {
			printf("error\n");
		} else {
			printf("opened!\n");
			break;
		}
	}
	
	if(nunchuk_fd < 0) {
		printf("could not open any adapter (/dev/i2c-x)\n");
		return -1;
	}
	
	usleep(100);
	
	if(ioctl(nunchuk_fd, I2C_SLAVE, NUNCHUK_ADDRESS) < 0) {
		printf("error with ioctl I2C_SLAVE\n");
		return -1;
	}
	
	//update_period = (1/(float)update_freq) * 1000 * 1000;
	
	nunchuk_ioctl_data.msgs = (struct i2c_msg *)malloc(sizeof(struct i2c_msg));
	nunchuk_ioctl_data.nmsgs = 1;
	nunchuk_ioctl_data.msgs->addr = NUNCHUK_ADDRESS;
	nunchuk_ioctl_data.msgs->flags = I2C_M_RD;
	nunchuk_ioctl_data.msgs->len = NUNCHUK_DATA_LENGTH;
	nunchuk_ioctl_data.msgs->buf = nunchuk_data_buffer;

	memset(nunchuk_data_buffer, 0, sizeof(uint8_t) * NUNCHUK_DATA_LENGTH);
	
	nunchuk_run_thread = 1;	
	pthread_create(&nunchuk_thread, NULL, nunchuk_loop, (void*) n);
	
	nunchuk_initiated = 1;
	usleep(100);
	return 1;
}


int nunchuk_read_data(struct nunchuk* n)
{
	/*ret = i2c_smbus_read_i2c_block_data(fd, 0, NUNCHUK_DATA_LENGTH, (uint8_t *)nunchuk_buffer);
	if(ret < 0) {
		//printf("error reading data\n");
	} else {
		printf("read %d byte(s)!!\n", ret);
	}*/

	int ret = ioctl(nunchuk_fd, I2C_RDWR, &nunchuk_ioctl_data);
	
	if(ret < 0) {
		printf("error reading data I2C_RDWR\n");
		nunchuk_try_callback(0);
		return -1;
	} else {
		nunchuk_request_data();	
		nunchuk_parse_data(n);
		nunchuk_try_callback(1);
	}

	/*int i;
	for(i = 0; i < NUNCHUK_DATA_LENGTH; i++)
		nunchuk_buffer[i] = i2c_smbus_read_byte(fd);*/

	return 1;	
}

void nunchuk_set_request_freq(int new_freq)
{
	if(new_freq < NUNCHUK_REQ_MIN_FREQ) {
		nunchuk_request_freq = NUNCHUK_REQ_MIN_FREQ;
	} else if(new_freq > NUNCHUK_REQ_MIN_FREQ) {
		nunchuk_request_freq = NUNCHUK_REQ_MAX_FREQ;
	} else {
		nunchuk_request_freq = new_freq;
	}
	nunchuk_calculate_request_period();
}

int nunchuk_get_request_freq()
{
	return nunchuk_request_freq;
}

void nunchuk_set_read_callback(nuncuk_read_callback func)
{
	nuncuk_read_callback_func = func;
}

int nunchuk_exit()
{
	nunchuk_return_not_initiated();
	nunchuk_run_thread = 0;
	pthread_cancel(nunchuk_thread);
	pthread_join(nunchuk_thread, NULL);
	if(nunchuk_ioctl_data.msgs == NULL) {
		free(nunchuk_ioctl_data.msgs);
	}
	if(nunchuk_fd > 0) {
		close(nunchuk_fd);
	}
	nunchuk_initiated = 0;
	return 1;
}



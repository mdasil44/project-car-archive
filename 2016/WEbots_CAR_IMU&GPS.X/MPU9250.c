/**
 * Invensense MPU-9250 library using the SPI interface
 *
 * Copyright (C) 2015 Brian Chen
 * Adapted to be used on dsPIC33 by Patrick Egan
 *
 * DSPIC33EP256MU806
 * Open source under the MIT License. See LICENSE.txt.
 */
#include "MPU9250.h"

/**
 * new_MPU9250: Creates a new MPU struct which saves incoming data from the MPU9250
 * mostly useful for calibration.
 *
 * @param low_pass_filter
 * @param low_pass_filter_acc
 * @return
 */
MPU9250* *new_MPU9250(unsigned char low_pass_filter = BITS_DLPF_CFG_188HZ, unsigned char low_pass_filter_acc = BITS_DLPF_CFG_188HZ) {
    MPU9250 *mpu = (MPU9250 *) malloc(sizeof(MPU9250));
    mpu->my_low_pass_filter = low_pass_filter;
    mpu->my_low_pass_filter_acc = low_pass_filter_acc;
    return mpu;
}

/**
 * init_SPI: Startup SPI communications to the MPU9250, must be run
 * before attempting to send or receive over SPI
 */
void init_SPI (void) {
    // setup the SPI peripheral
    SPI1STAT = 0x0;                             // disable the SPI module (just in case)
    SPI1CON1 = 0x0161;                          // FRAMEN = 0, SPIFSD = 0, DISSDO = 0, MODE16 = 0; SMP = 0; CKP = 1; CKE = 1; SSEN = 0; MSTEN = 1; SPRE = 0b000, PPRE = 0b01
    SPI1CON1bits.CKE = 0x01;
    SPI1CON1bits.CKP = 0x00;
    SPI1STAT = 0x8000;                          // enable the SPI module


    //TODO: Set clock to synchronize with Michael's hardware settings
    // Configure Oscillator to operate the device at 60Mhz
    // Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
    // Fosc= 8M*60/(2*2)=120Mhz for 8M input clock
    PLLFBD = 58;                                // M=60
    CLKDIVbits.PLLPOST = 0;                     // N1=2
    CLKDIVbits.PLLPRE = 0;                      // N2=2
    OSCTUN = 0;                                 // Tune FRC oscillator, if FRC is used

    // Disable Watch Dog Timer
    RCONbits.SWDTEN = 0;

    // Clock switching to incorporate PLL
    __builtin_write_OSCCONH( 0x03 );            // Initiate Clock Switch to Primary

    // Oscillator(XT) with PLL (NOSC=0b011)
    __builtin_write_OSCCONL( OSCCON || 0x01 );  // Start clock switching
    while( OSCCONbits.COSC != 0b011 );

    // Wait for Clock switch to occur
    // Wait for PLL to lock
    while( OSCCONbits.LOCK != 1 )
    { };

    //TODO: Map RPn bits according to Michael's used pins
    RPINR20 = 75;           //SPI data input is mapped to pin 45
    RPOR0bits.RP64R = 5;    //SPI data output mapped to pin 46
    RPOR0bits.RP65R = 6;    //SPI clock output mapped to pin 49
    RPOR1bits.RP66R = 7;    //SPI slave select mapped to pin 50
}

/**
 * WriteReg: Write out commands to the MPU9250 using
 *           SPI communication and return data returned
 *           by the MPU9250 into the SPI buffer
 *
 * @param WriteData Command to be sent to MPU9250
 * @return Data received in SPI buffer
 */
unsigned int WriteReg(signed int WriteAddr, signed int WriteData)
{
    //Used to clear out SPI buffer
    signed int temp_val;

    //TODO: Switch SPI constants to proper SPI port (currently SPI1)
    temp_val = SPI1BUF;     //Dummy read to clear SPIRBF flag

    SPI1BUF = WriteAddr;    //Write address to SPI which will be sent to MPU9250
    while( !SPI1STATbits.SPITBF );  // wait for the data to be sent out
    __delay_ms(50);         //Wait before sending data

    SPI1BUF = WriteData;    //Write data to SPI which will be sent to MPU9250
    while( !SPI1STATbits.SPITBF );  // wait for the data to be sent out

    while( !SPI1STATbits.SPIRBF);   // wait for MPU9250 to return data
    temp_val = SPI1BUF;     //Read value returned over SPI

    __delay_ms(50);
    return temp_val;
}

/**
 * ReadReg():
 *
 * @param WriteAddr
 * @param WriteData
 * @return
 */
unsigned int ReadReg(signed int WriteAddr, signed int WriteData)
{
    return WriteReg(WriteAddr | READ_FLAG, WriteData);
}


void ReadRegs(signed int ReadAddr, signed int *ReadBuf, signed int Bytes )
{
    unsigned int  i = 0;    //Counter variable
    signed int temp_val;    //

    temp_val = SPI1BUF;     //Dummy read to clear SPIRBF flag

    SPI1BUF = (ReadAddr | READ_FLAG);    //Write address to SPI which will be sent to MPU9250
    while( !SPI1STATbits.SPITBF );  // wait for the data to be sent out
    for(i = 0; i < Bytes; i++) {
        SPI1BUF = 0x00;    //Write data to SPI which will be sent to MPU9250
        while( !SPI1STATbits.SPITBF);  // wait for the data to be sent out
        while( !SPI1STATbits.SPIRBF);  // wait for MPU9250 to return data
        temp_val = SPI1BUF;     //Read value returned over SPI
    }
    __delay_ms(50);
}


/*                                     INITIALIZATION
 * usage: call this function at startup, giving the sample rate divider (raging from 0 to 255) and
 * low pass filter value; suitable values are:
 * BITS_DLPF_CFG_256HZ_NOLPF2
 * BITS_DLPF_CFG_188HZ
 * BITS_DLPF_CFG_98HZ
 * BITS_DLPF_CFG_42HZ
 * BITS_DLPF_CFG_20HZ
 * BITS_DLPF_CFG_10HZ
 * BITS_DLPF_CFG_5HZ
 * BITS_DLPF_CFG_2100HZ_NOLPF
 * returns 1 if an error occurred
 */

#define MPU_InitRegNum 17

bool init(MPU9250* mpu, bool calib_gyro, bool calib_acc){

/*
    pinMode(my_cs, OUTPUT);
#ifdef CORE_TEENSY
    digitalWriteFast(my_cs, HIGH);
#else
    digitalWrite(my_cs, HIGH);
#endif
 */
    float temp[3];

    if(calib_gyro && calib_acc){
        calibrate(mpu->g_bias, mpu->a_bias);
    }
    else if(calib_gyro){
        calibrate(mpu->g_bias, temp);
    }
    else if(calib_acc){
        calibrate(temp, mpu->a_bias);
    }

    unsigned char i = 0;
    unsigned char MPU_Init_Data[MPU_InitRegNum][2] = {
        {BIT_H_RESET, MPUREG_PWR_MGMT_1},     // Reset Device
        {0x01, MPUREG_PWR_MGMT_1},     // Clock Source
        {0x00, MPUREG_PWR_MGMT_2},     // Enable Acc & Gyro
        {mpu.my_low_pass_filter, MPUREG_CONFIG},         // Use DLPF set Gyroscope bandwidth 184Hz, temperature bandwidth 188Hz
        {BITS_FS_250DPS, MPUREG_GYRO_CONFIG},    // +-250dps
        {BITS_FS_2G, MPUREG_ACCEL_CONFIG},   // +-2G
        {mpu.my_low_pass_filter_acc, MPUREG_ACCEL_CONFIG_2}, // Set Acc Data Rates, Enable Acc LPF , Bandwidth 184Hz
        {0x30, MPUREG_INT_PIN_CFG},    //
        //{0x40, MPUREG_I2C_MST_CTRL},   // I2C Speed 348 kHz
        //{0x20, MPUREG_USER_CTRL},      // Enable AUX
        {0x20, MPUREG_USER_CTRL},       // I2C Master mode
        {0x0D, MPUREG_I2C_MST_CTRL}, //  I2C configuration multi-master  IIC 400KHz

        {AK8963_I2C_ADDR, MPUREG_I2C_SLV0_ADDR},  //Set the I2C slave addres of AK8963 and set for write.
        //{0x09, MPUREG_I2C_SLV4_CTRL},
        //{0x81, MPUREG_I2C_MST_DELAY_CTRL}, //Enable I2C delay

        {AK8963_CNTL2, MPUREG_I2C_SLV0_REG}, //I2C slave 0 register address from where to begin data transfer
        {0x01, MPUREG_I2C_SLV0_DO}, // Reset AK8963
        {0x81, MPUREG_I2C_SLV0_CTRL},  //Enable I2C and set 1 byte

        {AK8963_CNTL1, MPUREG_I2C_SLV0_REG}, //I2C slave 0 register address from where to begin data transfer
#ifdef AK8963FASTMODE
        {0x16, MPUREG_I2C_SLV0_DO}, // Register value to 100Hz continuous measurement in 16bit
#else
        {0x12, MPUREG_I2C_SLV0_DO}, // Register value to 8Hz continuous measurement in 16bit
#endif
        {0x81, MPUREG_I2C_SLV0_CTRL}  //Enable I2C and set 1 byte

    };

    for(i = 0; i < MPU_InitRegNum; i++) {
        WriteReg(MPU_Init_Data[i][1], MPU_Init_Data[i][0]);
        //I2C must slow down the write speed, otherwise it won't work
        __delay_us(1000);
    }

    set_acc_scale(BITS_FS_2G);
    set_gyro_scale(BITS_FS_250DPS);

    //calib_mag();  //Can't load this function here , strange problem?
    return 0;
}

/*                                ACCELEROMETER SCALE
 * usage: call this function at startup, after initialization, to set the right range for the
 * accelerometers. Suitable ranges are:
 * BITS_FS_2G
 * BITS_FS_4G
 * BITS_FS_8G
 * BITS_FS_16G
 * returns the range set (2,4,8 or 16)
 */

unsigned int set_acc_scale(MPU9250* mpu, int scale){
    unsigned int temp_scale;
    WriteReg(MPUREG_ACCEL_CONFIG, scale);

    switch (scale){
        case BITS_FS_2G:
            mpu->acc_divider=16384;
        break;
        case BITS_FS_4G:
            mpu->acc_divider=8192;
        break;
        case BITS_FS_8G:
            mpu->acc_divider=4096;
        break;
        case BITS_FS_16G:
            mpu->acc_divider=2048;
        break;
    }
    temp_scale = WriteReg(MPUREG_ACCEL_CONFIG|READ_FLAG, 0x00);

    switch (temp_scale){
        case BITS_FS_2G:
            temp_scale=2;
        break;
        case BITS_FS_4G:
            temp_scale=4;
        break;
        case BITS_FS_8G:
            temp_scale=8;
        break;
        case BITS_FS_16G:
            temp_scale=16;
        break;
    }
    return temp_scale;
}



/*                                 GYROSCOPE SCALE
 * usage: call this function at startup, after initialization, to set the right range for the
 * gyroscopes. Suitable ranges are:
 * BITS_FS_250DPS
 * BITS_FS_500DPS
 * BITS_FS_1000DPS
 * BITS_FS_2000DPS
 * returns the range set (250,500,1000 or 2000)
 */

unsigned int set_gyro_scale(MPU9250* mpu, int scale){
    unsigned int temp_scale;
    WriteReg(MPUREG_GYRO_CONFIG, scale);

    switch (scale){
        case BITS_FS_250DPS:   mpu->gyro_divider = 131;  break;
        case BITS_FS_500DPS:   mpu->gyro_divider = 65.5; break;
        case BITS_FS_1000DPS:  mpu->gyro_divider = 32.8; break;
        case BITS_FS_2000DPS:  mpu->gyro_divider = 16.4; break;
    }

    temp_scale = WriteReg(MPUREG_GYRO_CONFIG|READ_FLAG, 0x00);

    switch (temp_scale){
        case BITS_FS_250DPS:   temp_scale = 250;    break;
        case BITS_FS_500DPS:   temp_scale = 500;    break;
        case BITS_FS_1000DPS:  temp_scale = 1000;   break;
        case BITS_FS_2000DPS:  temp_scale = 2000;   break;
    }
    return temp_scale;
}



/*                                 WHO AM I?
 * usage: call this function to know if SPI is working correctly. It checks the I2C address of the
 * mpu9250 which should be 0x71
 */

unsigned int whoami(){
    unsigned int response;
    response = WriteReg(MPUREG_WHOAMI|READ_FLAG, 0x00);
    return response;
}



/*                                 READ ACCELEROMETER
 * usage: call this function to read accelerometer data. Axis represents selected axis:
 * 0 -> X axis
 * 1 -> Y axis
 * 2 -> Z axis
 */

void read_acc(MPU9250* mpu)
{
    unsigned char response[6];
    signed int bit_data;
    float data;
    int i;
    ReadRegs(MPUREG_ACCEL_XOUT_H,response,6);
    for(i = 0; i < 3; i++) {
        bit_data = ((signed int)response[i*2]<<8)|response[i*2+1];
        data = (float)bit_data;
        mpu->accel_data[i] = data/mpu->acc_divider - mpu->a_bias[i];
    }

}

/*                                 READ GYROSCOPE
 * usage: call this function to read gyroscope data. Axis represents selected axis:
 * 0 -> X axis
 * 1 -> Y axis
 * 2 -> Z axis
 */

void read_gyro(MPU9250* mpu)
{
    unsigned char response[6];
    signed int bit_data;
    float data;
    int i;
    ReadRegs(MPUREG_GYRO_XOUT_H,response,6);
    for(i = 0; i < 3; i++) {
        bit_data = ((signed int)response[i*2]<<8) | response[i*2+1];
        data = (float)bit_data;
        mpu->gyro_data[i] = data/(mpu->gyro_divider) - mpu->g_bias[i];
    }

}


/*                                 READ temperature
 * usage: call this function to read temperature data.
 * returns the value in ?C
 */

void read_temp(MPU9250* mpu){
    unsigned char response[2];
    signed int bit_data;
    float data;
    ReadRegs(MPUREG_TEMP_OUT_H,response,2);

    bit_data = ((signed int)response[0]<<8)|response[1];
    data = (float)bit_data;
    mpu->temperature = (data/340)+36.53;
    //deselect();
}

/*                                 READ ACCELEROMETER CALIBRATION
 * usage: call this function to read accelerometer data. Axis represents selected axis:
 * 0 -> X axis
 * 1 -> Y axis
 * 2 -> Z axis
 * returns Factory Trim value
 */

void calib_acc(MPU9250* mpu)
{
    unsigned char response[4];
    int temp_scale;
    //READ CURRENT ACC SCALE
    temp_scale=WriteReg(MPUREG_ACCEL_CONFIG|READ_FLAG, 0x00);
    set_acc_scale(BITS_FS_8G);
    //ENABLE SELF TEST need modify
    //temp_scale=WriteReg(MPUREG_ACCEL_CONFIG, 0x80>>axis);

    ReadRegs(MPUREG_SELF_TEST_X,response,4);
    mpu->calib_data[0] = ((response[0]&11100000)>>3) | ((response[3]&00110000)>>4);
    mpu->calib_data[1] = ((response[1]&11100000)>>3) | ((response[3]&00001100)>>2);
    mpu->calib_data[2] = ((response[2]&11100000)>>3) | ((response[3]&00000011));

    set_acc_scale(temp_scale);
}

unsigned char AK8963_whoami(){
    unsigned char response;
    WriteReg(MPUREG_I2C_SLV0_ADDR,AK8963_I2C_ADDR|READ_FLAG); //Set the I2C slave addres of AK8963 and set for read.
    WriteReg(MPUREG_I2C_SLV0_REG, AK8963_WIA); //I2C slave 0 register address from where to begin data transfer
    WriteReg(MPUREG_I2C_SLV0_CTRL, 0x81); //Read 1 byte from the magnetometer

    //WriteReg(MPUREG_I2C_SLV0_CTRL, 0x81);    //Enable I2C and set bytes
    __delay_ms(100);
    response = WriteReg(MPUREG_EXT_SENS_DATA_00|READ_FLAG, 0x00);    //Read I2C
    //ReadRegs(MPUREG_EXT_SENS_DATA_00,response,1);
    //response=WriteReg(MPUREG_I2C_SLV0_DO, 0x00);    //Read I2C

    return response;
}

void calib_mag(MPU9250* mpu){
    unsigned char response[3];
    float data;
    int i;

    WriteReg(MPUREG_I2C_SLV0_ADDR,AK8963_I2C_ADDR|READ_FLAG); //Set the I2C slave addres of AK8963 and set for read.
    WriteReg(MPUREG_I2C_SLV0_REG, AK8963_ASAX); //I2C slave 0 register address from where to begin data transfer
    WriteReg(MPUREG_I2C_SLV0_CTRL, 0x83); //Read 3 bytes from the magnetometer

    //WriteReg(MPUREG_I2C_SLV0_CTRL, 0x81);    //Enable I2C and set bytes
    __delay_ms(1000);
    //response[0]=WriteReg(MPUREG_EXT_SENS_DATA_01|READ_FLAG, 0x00);    //Read I2C
    ReadRegs(MPUREG_EXT_SENS_DATA_00,response,3);

    //response=WriteReg(MPUREG_I2C_SLV0_DO, 0x00);    //Read I2C
    for(i = 0; i < 3; i++) {
        data=response[i];
        mpu->Magnetometer_ASA[i] = ((data-128)/256+1)*Magnetometer_Sensitivity_Scale_Factor;
    }
}
void read_mag(MPU9250* mpu){
    unsigned char response[7];
    float data;
    int i;

    WriteReg(MPUREG_I2C_SLV0_ADDR,AK8963_I2C_ADDR|READ_FLAG); //Set the I2C slave addres of AK8963 and set for read.
    WriteReg(MPUREG_I2C_SLV0_REG, AK8963_HXL); //I2C slave 0 register address from where to begin data transfer
    WriteReg(MPUREG_I2C_SLV0_CTRL, 0x87); //Read 6 bytes from the magnetometer

    __delay_ms(1000);
    ReadRegs(MPUREG_EXT_SENS_DATA_00,response,7);
    //must start your read from AK8963A register 0x03 and read seven bytes so that upon read of ST2 register 0x09 the AK8963A will unlatch the data registers for the next measurement.
    for(i = 0; i < 3; i++) {
        mpu->mag_data_raw[i] = ((signed int)response[i*2+1]<<8)|response[i*2];
        data = (float) mpu->mag_data_raw[i];
        mpu->mag_data[i] = data*(mpu->Magnetometer_ASA[i]);
    }
}

unsigned char get_CNTL1(){
    WriteReg(MPUREG_I2C_SLV0_ADDR,AK8963_I2C_ADDR|READ_FLAG); //Set the I2C slave addres of AK8963 and set for read.
    WriteReg(MPUREG_I2C_SLV0_REG, AK8963_CNTL1); //I2C slave 0 register address from where to begin data transfer
    WriteReg(MPUREG_I2C_SLV0_CTRL, 0x81); //Read 1 byte from the magnetometer

    __delay_ms(1000);
    return WriteReg(MPUREG_EXT_SENS_DATA_00|READ_FLAG, 0x00);    //Read I2C
}

void read_all(MPU9250 *mpu){
    unsigned char response[21];
    signed int bit_data;
    float data;
    int i;

    //Send I2C command at first
    WriteReg(MPUREG_I2C_SLV0_ADDR,AK8963_I2C_ADDR|READ_FLAG); //Set the I2C slave addres of AK8963 and set for read.
    WriteReg(MPUREG_I2C_SLV0_REG, AK8963_HXL); //I2C slave 0 register address from where to begin data transfer
    WriteReg(MPUREG_I2C_SLV0_CTRL, 0x87); //Read 7 bytes from the magnetometer
    //must start your read from AK8963A register 0x03 and read seven bytes so that upon read of ST2 register 0x09 the AK8963A will unlatch the data registers for the next measurement.

    ReadRegs(MPUREG_ACCEL_XOUT_H,response,21);
    //Get accelerometer value
    for(i = 0; i < 3; i++) {
        bit_data = ((signed int)response[i*2]<<8) | response[i*2+1];
        data = (float)bit_data;
        mpu->accel_data[i] = data/mpu->acc_divider - mpu->a_bias[i];
    }
    //Get temperature
    bit_data = ((signed int)response[i*2]<<8) | response[i*2+1];
    data = (float)bit_data;
    mpu->temperature = ((data-21)/333.87)+21;
    //Get gyroscope value
    for(i=4; i < 7; i++) {
        bit_data = ((signed int)response[i*2]<<8) | response[i*2+1];
        data = (float)bit_data;
        mpu->gyro_data[i-4] = data/mpu->gyro_divider - mpu->g_bias[i-4];
    }
    //Get Magnetometer value
    for(i=7; i < 10; i++) {
        mpu->mag_data_raw[i] = ((signed int)response[i*2+1]<<8) | response[i*2];
        data = (float) mpu->mag_data_raw[i];
        mpu->mag_data[i-7] = data * mpu->Magnetometer_ASA[i-7];
    }
}

void calibrate(float *dest1, float *dest2){
    unsigned char data[12]; // data array to hold accelerometer and gyro x, y, z, data
    unsigned int ii, packet_count, fifo_count;
    signed long gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};

    // reset device
    WriteReg(MPUREG_PWR_MGMT_1, 0x80); // Write a one to bit 7 reset bit; toggle reset device
    //delay(100);
    __delay_ms(100);

    // get stable time source; Auto select clock source to be PLL gyroscope reference if ready
    // else use the internal oscillator, bits 2:0 = 001
    WriteReg(MPUREG_PWR_MGMT_1, 0x01);
    WriteReg(MPUREG_PWR_MGMT_2, 0x00);
    //delay(200);
    __delay_ms(200);

    // Configure device for bias calculation
    WriteReg(MPUREG_INT_ENABLE, 0x00);   // Disable all interrupts
    WriteReg(MPUREG_FIFO_EN, 0x00);      // Disable FIFO
    WriteReg(MPUREG_PWR_MGMT_1, 0x00);   // Turn on internal clock source
    WriteReg(MPUREG_I2C_MST_CTRL, 0x00); // Disable I2C master
    WriteReg(MPUREG_USER_CTRL, 0x00);    // Disable FIFO and I2C master modes
    WriteReg(MPUREG_USER_CTRL, 0x0C);    // Reset FIFO and DMP
    //delay(15);
    __delay_ms(15);

    // Configure MPU6050 gyro and accelerometer for bias calculation
    WriteReg(MPUREG_CONFIG, 0x01);      // Set low-pass filter to 188 Hz
    WriteReg(MPUREG_SMPLRT_DIV, 0x00);  // Set sample rate to 1 kHz
    WriteReg(MPUREG_GYRO_CONFIG, 0x00);  // Set gyro full-scale to 250 degrees per second, maximum sensitivity
    WriteReg(MPUREG_ACCEL_CONFIG, 0x00); // Set accelerometer full-scale to 2 g, maximum sensitivity

    unsigned int  gyrosensitivity  = 131;   // = 131 LSB/degrees/sec
    unsigned int  accelsensitivity = 16384;  // = 16384 LSB/g

      // Configure FIFO to capture accelerometer and gyro data for bias calculation
    WriteReg(MPUREG_USER_CTRL, 0x40);   // Enable FIFO
    WriteReg(MPUREG_FIFO_EN, 0x78);     // Enable gyro and accelerometer sensors for FIFO  (max size 512 bytes in MPU-9150)
    // accumulate 40 samples in 40 milliseconds = 480 bytes
    //delay(40);
    __delay_ms(40);

    // At end of sample accumulation, turn off FIFO sensor read
    WriteReg(MPUREG_FIFO_EN, 0x00);        // Disable gyro and accelerometer sensors for FIFO
    ReadRegs(MPUREG_FIFO_COUNTH, data, 2); // read FIFO sample count
    fifo_count = ((unsigned int)data[0] << 8) | data[1];
    packet_count = fifo_count/12;// How many sets of full gyro and accelerometer data for averaging

    for (ii = 0; ii < packet_count; ii++) {
        signed int accel_temp[3] = {0, 0, 0}, gyro_temp[3] = {0, 0, 0};
        ReadRegs(MPUREG_FIFO_R_W, data, 12); // read data for averaging
        accel_temp[0] = (signed int) (((signed int)data[0] << 8) | data[1]  ) ;  // Form signed 16-bit integer for each sample in FIFO
        accel_temp[1] = (signed int) (((signed int)data[2] << 8) | data[3]  ) ;
        accel_temp[2] = (signed int) (((signed int)data[4] << 8) | data[5]  ) ;
        gyro_temp[0]  = (signed int) (((signed int)data[6] << 8) | data[7]  ) ;
        gyro_temp[1]  = (signed int) (((signed int)data[8] << 8) | data[9]  ) ;
        gyro_temp[2]  = (signed int) (((signed int)data[10] << 8) | data[11]) ;

        accel_bias[0] += (signed long) accel_temp[0]; // Sum individual signed 16-bit biases to get accumulated signed 32-bit biases
        accel_bias[1] += (signed long) accel_temp[1];
        accel_bias[2] += (signed long) accel_temp[2];
        gyro_bias[0]  += (signed long) gyro_temp[0];
        gyro_bias[1]  += (signed long) gyro_temp[1];
        gyro_bias[2]  += (signed long) gyro_temp[2];

    }
    accel_bias[0] /= (signed long) packet_count; // Normalize sums to get average count biases
    accel_bias[1] /= (signed long) packet_count;
    accel_bias[2] /= (signed long) packet_count;
    gyro_bias[0]  /= (signed long) packet_count;
    gyro_bias[1]  /= (signed long) packet_count;
    gyro_bias[2]  /= (signed long) packet_count;

    if(accel_bias[2] > 0L) {accel_bias[2] -= (signed long) accelsensitivity;}  // Remove gravity from the z-axis accelerometer bias calculation
    else {accel_bias[2] += (signed long) accelsensitivity;}

    // Construct the gyro biases for push to the hardware gyro bias registers, which are reset to zero upon device startup
    data[0] = (-gyro_bias[0]/4  >> 8) & 0xFF; // Divide by 4 to get 32.9 LSB per deg/s to conform to expected bias input format
    data[1] = (-gyro_bias[0]/4)       & 0xFF; // Biases are additive, so change sign on calculated average gyro biases
    data[2] = (-gyro_bias[1]/4  >> 8) & 0xFF;
    data[3] = (-gyro_bias[1]/4)       & 0xFF;
    data[4] = (-gyro_bias[2]/4  >> 8) & 0xFF;
    data[5] = (-gyro_bias[2]/4)       & 0xFF;

    // Push gyro biases to hardware registers
    WriteReg(MPUREG_XG_OFFS_USRH, data[0]);
    WriteReg(MPUREG_XG_OFFS_USRL, data[1]);
    WriteReg(MPUREG_YG_OFFS_USRH, data[2]);
    WriteReg(MPUREG_YG_OFFS_USRL, data[3]);
    WriteReg(MPUREG_ZG_OFFS_USRH, data[4]);
    WriteReg(MPUREG_ZG_OFFS_USRL, data[5]);

    // Output scaled gyro biases for display in the main program
    dest1[0] = (float) gyro_bias[0]/(float) gyrosensitivity;
    dest1[1] = (float) gyro_bias[1]/(float) gyrosensitivity;
    dest1[2] = (float) gyro_bias[2]/(float) gyrosensitivity;

    // Construct the accelerometer biases for push to the hardware accelerometer bias registers. These registers contain
    // factory trim values which must be added to the calculated accelerometer biases; on boot up these registers will hold
    // non-zero values. In addition, bit 0 of the lower byte must be preserved since it is used for temperature
    // compensation calculations. Accelerometer bias registers expect bias input as 2048 LSB per g, so that
    // the accelerometer biases calculated above must be divided by 8.

    signed long accel_bias_reg[3] = {0, 0, 0}; // A place to hold the factory accelerometer trim biases
    ReadRegs(MPUREG_XA_OFFSET_H, data, 2); // Read factory accelerometer trim values
    accel_bias_reg[0] = (signed long) (((signed int)data[0] << 8) | data[1]);
    ReadRegs(MPUREG_YA_OFFSET_H, data, 2);
    accel_bias_reg[1] = (signed long) (((signed int)data[0] << 8) | data[1]);
    ReadRegs(MPUREG_ZA_OFFSET_H, data, 2);
    accel_bias_reg[2] = (signed long) (((signed int)data[0] << 8) | data[1]);

    unsigned long mask = 1uL; // Define mask for temperature compensation bit 0 of lower byte of accelerometer bias registers
    unsigned char mask_bit[3] = {0, 0, 0}; // Define array to hold mask bit for each accelerometer bias axis

    for(int ii = 0; ii < 3; ii++) {
      if((accel_bias_reg[ii] & mask)) mask_bit[ii] = 0x01; // If temperature compensation bit is set, record that fact in mask_bit
    }

    // Construct total accelerometer bias, including calculated average accelerometer bias from above
    accel_bias_reg[0] -= (accel_bias[0]/8); // Subtract calculated averaged accelerometer bias scaled to 2048 LSB/g (16 g full scale)
    accel_bias_reg[1] -= (accel_bias[1]/8);
    accel_bias_reg[2] -= (accel_bias[2]/8);

    data[0] = (accel_bias_reg[0] >> 8) & 0xFF;
    data[1] = (accel_bias_reg[0])      & 0xFF;
    data[1] = data[1] | mask_bit[0]; // preserve temperature compensation bit when writing back to accelerometer bias registers
    data[2] = (accel_bias_reg[1] >> 8) & 0xFF;
    data[3] = (accel_bias_reg[1])      & 0xFF;
    data[3] = data[3] | mask_bit[1]; // preserve temperature compensation bit when writing back to accelerometer bias registers
    data[4] = (accel_bias_reg[2] >> 8) & 0xFF;
    data[5] = (accel_bias_reg[2])      & 0xFF;
    data[5] = data[5] | mask_bit[2]; // preserve temperature compensation bit when writing back to accelerometer bias registers

// Apparently this is not working for the acceleration biases in the MPU-9250
// Are we handling the temperature correction bit properly?
// Push accelerometer biases to hardware registers
    WriteReg(MPUREG_XA_OFFSET_H, data[0]);
    WriteReg(MPUREG_XA_OFFSET_L, data[1]);
    WriteReg(MPUREG_YA_OFFSET_H, data[2]);
    WriteReg(MPUREG_YA_OFFSET_L, data[3]);
    WriteReg(MPUREG_ZA_OFFSET_H, data[4]);
    WriteReg(MPUREG_ZA_OFFSET_L, data[5]);

// Output scaled accelerometer biases for display in the main program
    dest2[0] = (float)accel_bias[0]/(float)accelsensitivity;
    dest2[1] = (float)accel_bias[1]/(float)accelsensitivity;
    dest2[2] = (float)accel_bias[2]/(float)accelsensitivity;
}

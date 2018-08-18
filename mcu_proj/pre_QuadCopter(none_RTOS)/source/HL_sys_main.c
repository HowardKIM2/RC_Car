
#include "MPU9250.h"
#include "MPU6050.h"
#include "Matrix.h"
#include "QuadCopter.h"


max1 torque;  // ���� ��ũ
max1 v_dot; // ��ü��ǥ�� ���ӵ�
max1 eta_ref; // �����ӵ�
max1 p_2dot; // ���ӵ�

int i;

int main(void)
{

    char txt_buf[256] = {0};
    unsigned int buf_len;

    sciInit();

    disp_set("SCI Configuration Success!!\n\r\0");

    gioInit();

    disp_set("GIO Init Success!!\n\r\0");

    i2cInit();
    wait(10000000);
    disp_set("I2C Init Success!!\n\r\0");


    etpwmInit();

    sprintf(txt_buf, "ETPWM Configuration Success!!\n\r");
    buf_len = strlen(txt_buf);
    sciDisplayText(UART, (uint8 *) txt_buf, buf_len);

    etpwmStartTBCLK();
    wait(10000);

    dt = 0.0025;



//
//        initDT();
//
//    sprintf(txt_buf, "Please Press key!!(0 ~ 5)\n\r");
//    buf_len = strlen(txt_buf);
//    sciDisplayText(UART, (uint8 *) txt_buf, buf_len);

    uint8 c = readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250); // MPU9250 �ν��� �Ǿ� ����� ���� �о��� ���� 0x71�� ����.
    sprintf(txt_buf, "I AM = %x\n\r\0", c);
    buf_len = strlen(txt_buf);
    sciDisplayText(sciREG1, (uint8 *) txt_buf, buf_len);

     if (c == 0x71)
     {
         calibrateMPU9250(gyroBias, accelBias); // gyro, acc ��հ� ������ ���� �Լ�
         disp_set("MPU9250 calibration Success!!!!!!\n\r\0");

         initMPU9250(); // MPU9250 �ʱ�ȭ
         disp_set("MPU9250 Init Success!!!!!!\n\r\0");

         initAK8963(magCalibration); // ���ڱ� �ʱ�ȭ
         disp_set("MPU9250 AK8963 Init Success!!!!!!\n\r\0");

         get_offset_value(); // gyro ���� 1000���� �о� ��հ��� ���ϴ� �Լ�
         disp_set("gyro_offset_setting Success!!\n\r\0");

         initMotorPWM();
         disp_set("Init MOTOR_PWM Success!!\n\r\0");

         initYPR();
         get_Iv();
         get_inv_T();

         /*�� ���� �� �� �Ű��� �� �κ���. �ӵ� ���� �˾ƾ� ��. MPU9250 ���ӵ����� �����ؼ� ���� �� �� ������.*/
         p_2dot.x =0;
         p_2dot.y =0;
         p_2dot.z =0;

         disp_set("Init YPR Success!!\n\r\0");

         wait(1000000);
     }else{
         disp_set("MPU9250 doesn`t work!!!");
         while(1);
     }


     for (;;)   // loop()
     {
         /*mpu6050 �ؿ� ���̷ο� ������ Ȯ���� ��.*/
//         get_mpu6050_RPY();
//         get_mpu6050_gyro();


         if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01) // ���ͷ�Ʈ�� Ȱ��ȭ Ȯ��
         {

             readAccelData(accelCount);  // Read the x/y/z adc values
             getAres();  // scale�� ������ִ� �Լ�

             ax = (float) accelCount[0] * aRes; // - accelBias[0];  // get actual g value, this depends on scale being set
             ay = (float) accelCount[1] * aRes; // - accelBias[1];
             az = (float) accelCount[2] * aRes; // - accelBias[2];

             readGyroData(gyroCount);  // Read the x/y/z adc values
             getGres();

             // Calculate the gyro value into actual degrees per second
             gx = (float) gyroCount[0] * gRes; // get actual gyro value, this depends on scale being set
             gy = (float) gyroCount[1] * gRes;
             gz = (float) gyroCount[2] * gRes;


             /*roll(x)�� pitch(y) yaw(z) �� ����*/
              get_YPR();

//                           sprintf(txt_buf,
//                                   "roll = %d \t pitch = %d \t yaw = %d \n\r\0",
//                                   (int)angle_pitch_acc, (int)angle_roll_acc, (int)yaw);
//
//                           buf_len = strlen(txt_buf);
//                           sciDisplayText(sciREG1, (uint8 *) txt_buf, buf_len);
//                           wait(3000000);

              /*sensor angle & gyro & gyro_dot for mpu9250*/
             roll_angle_in = roll;
             roll_rate_in = gx;
             pitch_angle_in = pitch;
             pitch_rate_in = gy;
             yaw_angle_in = yaw;
             yaw_rate_in = gz;
             eta_ref.x = roll_output;
             eta_ref.y = pitch_output;
             eta_ref.z = yaw_output;

             /*QuadCopter Math & PWM for mpu9250*/
             calcYPRtoDualPID();
             get_C(roll,pitch, yaw , &C);

//            sprintf(txt_buf, "C1 = %f \t C2 = %f \t C3= %f \n\r  C4 = %f \t C5 = %f \t C6= %f \n\r  C7 = %f \t C8 = %f \t C9= %f \n\r\n\r",
//                    C.x[0],C.x[1],C.x[2],C.y[0],C.y[1],C.y[2],C.z[0],C.z[1],C.z[2]);
//            buf_len = strlen(txt_buf);
//            sciDisplayText(UART, (uint8 *) txt_buf, buf_len);
             get_t_R(roll, pitch, yaw, &t_R);
//                        sprintf(txt_buf, "t1 = %f \t t2 = %f \t t3= %f \n\r  t4 = %f \t t5 = %f \t t6= %f \n\r  t7 = %f \t t8 = %f \t t9= %f \n\r\n\r",
//                                t_R.x[0], t_R.x[1], t_R.x[2],
//                                t_R.y[0], t_R.y[1], t_R.y[2],
//                                t_R.z[0], t_R.z[1], t_R.z[2]);
//                        buf_len = strlen(txt_buf);
//                        sciDisplayText(UART, (uint8 *) txt_buf, buf_len);

             torque = torque_conv(Iv, C, eta_ref);
//                          sprintf(txt_buf, "t1 = %f \t t2 = %f \t t3 = %f \n\r", torque.x, torque.y, torque.z);
//                          buf_len = strlen(txt_buf);
//                          sciDisplayText(UART, (uint8 *) txt_buf, buf_len);
            v_dot = force_conv(t_R, p_2dot);


            get_motor_thrust(torque, v_dot,inv_T, U,F);


//            for(i=0; i<16;i++){
//                if (i % 4 == 0)
//                {
//                    sprintf(txt_buf, "\n\r");
//                    buf_len = strlen(txt_buf);
//                    sciDisplayText(UART, (uint8 *) txt_buf, buf_len);
//                }
//                sprintf(txt_buf, "inv[%d] = %f \t", i, inv_T[i]);
//                buf_len = strlen(txt_buf);
//                sciDisplayText(UART, (uint8 *) txt_buf, buf_len);
//            }

//
//             sprintf(txt_buf, "F0 = %f \t F1 = %f \t F2 = %f \t F3 = %f\n\r", F[0], F[1], F[2], F[3]);
//             buf_len = strlen(txt_buf);
//             sciDisplayText(UART, (uint8 *) txt_buf, buf_len);

            calcMotorPWM();


         }



     }



}


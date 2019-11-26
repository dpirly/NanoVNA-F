/*-----------------------------------------------------------------------------/
 * Module       : board.c
 * Create       : 2019-05-23
 * Copyright    : hamelec.taobao.com
 * Author       : huanglong
 * Brief        : 
/-----------------------------------------------------------------------------*/
#include "system.h"
#include "board.h"

#define I2C_WR                   0    // д���� bit
#define I2C_RD                   1    // ������ bit

#define I2C_GPIO_CLK_ENABLE()    __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2C_GPIO_PORT            GPIOB
#define I2C_SCL_PIN              GPIO_PIN_8
#define I2C_SDA_PIN              GPIO_PIN_9

#define I2C_SCL_HIGH()           HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_SET)
#define I2C_SCL_LOW()            HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SCL_PIN, GPIO_PIN_RESET)
#define I2C_SDA_HIGH()           HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_SET)
#define I2C_SDA_LOW()            HAL_GPIO_WritePin(I2C_GPIO_PORT, I2C_SDA_PIN, GPIO_PIN_RESET)
#define I2C_SDA_READ()           HAL_GPIO_ReadPin(I2C_GPIO_PORT, I2C_SDA_PIN)

extern TIM_HandleTypeDef htim1;

void    I2C_InitGPIO(void);
uint8_t I2C_CheckDevice(uint8_t addr);

/*
=======================================
    ϵͳ�ж�
=======================================
*/
void systick_call(void)
{

}

/*
=======================================
    I2C ��ʱ
=======================================
*/
static void I2C_Delay(void)
{
  uint8_t i;
  for (i = 0; i < 15; i++);  // STM32F103VET6 ��Ƶ 72MHz MDK -O3 �Ż���ʵ�� 7us��I2C CLK = 142KHZ
  // for (i = 0; i < 20; i++);  // STM32F103VET6 ��Ƶ 72MHz MDK -O3 �Ż������� 9us��I2C CLK = 100KHZ
  // for (i = 0; i < 50; i++);  // STM32F103VET6 ��Ƶ 72MHz MDK -O3 �Ż������� 20us��I2C CLK = 50KHZ
}

/*
=======================================
    I2C ����
=======================================
*/
void I2C_Test(void)
{
  // LED_TOG;
  I2C_Delay();
}

/*
=======================================
    CPU����I2C���������ź�
=======================================
*/
void I2C_Start(void)
{
  /* ��SCL�ߵ�ƽʱ��SDA����һ�������ر�ʾI2C���������ź� */
  I2C_SDA_HIGH();
  I2C_SCL_HIGH();
  I2C_Delay();
  I2C_SDA_LOW();
  I2C_Delay();
  I2C_SCL_LOW();
  I2C_Delay();
}

/*
=======================================
    CPU����I2C����ֹͣ�ź�
=======================================
*/
void I2C_Stop(void)
{
  /* ��SCL�ߵ�ƽʱ��SDA����һ�������ر�ʾI2C����ֹͣ�ź� */
  I2C_SDA_LOW();
  I2C_SCL_HIGH();
  I2C_Delay();
  I2C_SDA_HIGH();
}

/*
=======================================
    CPU����һ��ACK�ź�
=======================================
*/
void I2C_Ack(void)
{
  I2C_SCL_LOW();
  __NOP();
  I2C_SDA_LOW();     /* CPU����SDA = 0 */
  I2C_Delay();
  I2C_SCL_HIGH();    /* CPU����1��ʱ�� */
  I2C_Delay();
  I2C_SCL_LOW();
  I2C_Delay();
  I2C_SDA_HIGH();    /* CPU�ͷ�SDA���� */
}

/*
=======================================
    CPU����1��NACK�ź�
=======================================
*/
void I2C_NAck(void)
{
  I2C_SCL_LOW();
  __NOP();
  I2C_SDA_HIGH();    /* CPU����SDA = 1 */
  I2C_Delay();
  I2C_SCL_HIGH();    /* CPU����1��ʱ�� */
  I2C_Delay();
  I2C_SCL_LOW();
  I2C_Delay();
}

/*
=======================================
    CPU��I2C�����豸����8bit����
=======================================
*/
void I2C_SendByte(uint8_t Byte)
{
  uint8_t i;

  /* �ȷ����ֽڵĸ�λbit7 */
  for (i = 0; i < 8; i++)
  {    
    if (Byte & 0x80)
    {
      I2C_SDA_HIGH();
    } else {
      I2C_SDA_LOW();
    }
    I2C_Delay();
    I2C_SCL_HIGH();
    I2C_Delay();    
    I2C_SCL_LOW();
    if (i == 7)
    {
      I2C_SDA_HIGH();  /* �ͷ����� */
    }
    Byte <<= 1;    /* ����һ��bit */
    I2C_Delay();
  }
}

/*
=======================================
    CPU��I2C�����豸��ȡ8bit����
=======================================
*/
uint8_t I2C_ReadByte(uint8_t ack)
{
  uint8_t i;
  uint8_t value;

  /* ������1��bitΪ���ݵ�bit7 */
  value = 0;
  for (i = 0; i < 8; i++)
  {
    value <<= 1;
    I2C_SCL_HIGH();
    I2C_Delay();
    if (I2C_SDA_READ())
    {
      value++;
    }
    I2C_SCL_LOW();
    I2C_Delay();
  }

  if (!ack)
    I2C_NAck();  // ���� nACK
  else
    I2C_Ack();   // ���� ACK

  return value;
}

/*
=======================================
    CPU����һ��ʱ�ӣ�����ȡ������ACKӦ���ź�
    ����0��ʾ��ȷӦ��1��ʾ��������Ӧ
=======================================
*/
uint8_t I2C_WaitAck(void)
{
  uint8_t re;

  I2C_SDA_HIGH();    /* CPU�ͷ�SDA���� */
  I2C_Delay();

  I2C_SCL_HIGH();    /* CPU����SCL = 1, ��ʱ�����᷵��ACKӦ�� */
  I2C_Delay();
  
  if (I2C_SDA_READ())    /* CPU��ȡSDA����״̬ */
  {
    re = 1;
  } else {
    re = 0;
  }
  I2C_SCL_LOW();
  I2C_Delay();

  return re;
}

/*
=======================================
    I2C ��ʼ��
=======================================
*/
void I2C_InitGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* ��GPIOʱ�� */
  I2C_GPIO_CLK_ENABLE();

  GPIO_InitStruct.Pin = I2C_SCL_PIN|I2C_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(I2C_GPIO_PORT, &GPIO_InitStruct);

  /* ��һ��ֹͣ�ź�, ��λI2C�����ϵ������豸������ģʽ */
  I2C_Stop();
}

/*
=======================================
    ���I2C�����豸��CPU�����豸��ַ��Ȼ���ȡ�豸Ӧ�����жϸ��豸�Ƿ����
    addr���豸��I2C���ߵ�ַ
    ����ֵ 0 ��ʾ��ȷ�� ����1��ʾδ̽�⵽
    �ڷ���I2C�豸ǰ�����ȵ��� I2C_CheckDevice() ���I2C�豸�Ƿ�����
=======================================
*/
uint8_t I2C_CheckDevice(uint8_t addr)
{
  uint8_t ucAck;

  I2C_Start();              /* ���������ź� */
  /* �����豸��ַ+��д����bit��0 = w�� 1 = r) bit7 �ȴ� */
  I2C_SendByte((addr << 1) | 0);
  ucAck = I2C_WaitAck();    /* ����豸��ACKӦ�� */
  I2C_Stop();               /* ����ֹͣ�ź� */

  return ucAck;
}

/*
=======================================
    I2C д�����Ĵ���
=======================================
*/
int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t dat)
{
  // CPU���Ϳ�ʼλ
  I2C_Start();
  // ����ӵ�ַ����R/Wλ�趨дģʽλ�趨дģʽ
  I2C_SendByte(addr << 1);  // 0 = W
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // ����Ҫд�� reg
  I2C_SendByte(reg);
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU��Ҫд�������д��ָ���ļĴ���
  I2C_SendByte(dat);
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU����ֹͣλ
  I2C_Stop();
  return 1;

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
  /* ����I2C����ֹͣ�ź� */
  I2C_Stop();
  return 0;
}

/*
=======================================
    I2C д����Ĵ���
=======================================
*/
int i2c_write_regs(uint8_t addr, uint8_t reg, uint8_t *dat, uint8_t cnt)
{
  // CPU���Ϳ�ʼλ
  I2C_Start();
  // ����ӵ�ַ����R/Wλ�趨дģʽλ�趨дģʽ
  I2C_SendByte(addr << 1);  // 0 = W
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // ����Ҫд�� reg
  I2C_SendByte(reg);
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU��Ҫд�������д��ָ���ļĴ���
  while (cnt --)
  {
    I2C_SendByte(*(dat ++));
    if (I2C_WaitAck() != 0) {
      goto cmd_fail;
    }
  }

  // CPU����ֹͣλ
  I2C_Stop();
  return 1;

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
  /* ����I2C����ֹͣ�ź� */
  I2C_Stop();
  return 0;
}

/*
=======================================
    I2C �������Ĵ���
=======================================
*/
uint8_t i2c_read_reg(uint8_t addr, uint8_t reg)
{
  uint8_t dat = 0x00;

  // CPU���Ϳ�ʼλ
  I2C_Start();
  // ����ӵ�ַ����R/Wλ�趨дģʽλ�趨дģʽ
  I2C_SendByte(addr << 1);  // 0 = W
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // ����Ҫ���� reg
  I2C_SendByte(reg);
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU����RESTART����
  I2C_Start();
  // ����ӵ�ַ����R/Wλ�趨дģʽλ�趨��ģʽ
  I2C_SendByte((addr << 1) | 0x01);  // 1 = R
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU��ȡָ���ļĴ�������
  dat = I2C_ReadByte(0);
  // CPU����ֹͣλ
  I2C_Stop();
  return dat;

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
  /* ����I2C����ֹͣ�ź� */
  I2C_Stop();
  return 0;
}

/*
=======================================
    I2C д����Ĵ���
=======================================
*/
int i2c_read_regs(uint8_t addr, uint8_t reg, uint8_t *dat, uint8_t cnt)
{
  // CPU���Ϳ�ʼλ
  I2C_Start();
  // ����ӵ�ַ����R/Wλ�趨дģʽλ�趨дģʽ
  I2C_SendByte(addr << 1);  // 0 = W
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // ����Ҫ���� reg
  I2C_SendByte(reg);
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU����RESTART����
  I2C_Start();
  // ����ӵ�ַ����R/Wλ�趨дģʽλ�趨��ģʽ
  I2C_SendByte((addr << 1) | 0x01);  // 1 = R
  // �ȴ�����Ӧ��
  if (I2C_WaitAck() != 0) {
    goto cmd_fail;
  }
  // CPU��ȡָ���ļĴ�������
  while (cnt --)
  {
    *(dat ++) = I2C_ReadByte(0);
  }
  // CPU����ֹͣλ
  I2C_Stop();
  return 1;

cmd_fail: /* ����ִ��ʧ�ܺ��мǷ���ֹͣ�źţ�����Ӱ��I2C�����������豸 */
  /* ����I2C����ֹͣ�ź� */
  I2C_Stop();
  return 0;
}

/*
=======================================
    16�����ַ�ת����
=======================================
*/
uint8_t c2i(char ch)
{
  /* ��������֣��������ֵ� ASCII ���ȥ 48 */
  if ((ch <= '9') && (ch >= '0'))
    return (ch - 48);
  /* ����Ǵ�д��ĸ���������ֵ� ASCII ���ȥ 55 */
  if ((ch <= 'F') && (ch >= 'A'))
    return (ch - 55);
  /* �����Сд��ĸ���������ֵ� ASCII ���ȥ 87 */
  if ((ch <= 'f') && (ch >= 'a'))
    return (ch - 87);
  return 0;
}

/*
=======================================
    16�����ַ���ת���飬����ת�������鳤��
=======================================
*/
int str2hex(uint8_t *dst, char *src)
{
  int cnt, len;
  len = strlen(src);
  if (len % 2)
    return 0;
  for (cnt=0; cnt<(len/2); cnt++)
  {
    dst[cnt] = c2i(src[cnt*2])*16 + c2i(src[cnt*2+1]);
  }
  return cnt;
}

int g_BeepMs = 0;
/*
=======================================
    ��������ʱ��
=======================================
*/
void beep_open(int ms)
{
  g_BeepMs = ms;
  if (g_BeepMs > 0) {
    BEEP_ON();
  }
}

/*
=======================================
    ��������ʱ��
=======================================
*/
void beep_tick(void)
{
  if (g_BeepMs > 0) {
    g_BeepMs --;
    if (g_BeepMs == 0) {
      BEEP_OFF();
    }
  }
}

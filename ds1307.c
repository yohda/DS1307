#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

/* mdelay() */
#include <linux/delay.h>

#include <linux/rtc.h>
#include <linux/bcd.h>

#include <linux/regmap.h>

enum {
	DS1307_RTC_SECONDS_REG_ADDR = 0x00,
 	DS1307_RTC_MINUTES_REG_ADDR,			
 	DS1307_RTC_HOURS_REG_ADDR,			
 	DS1307_RTC_DAY_REG_ADDR,				
 	DS1307_RTC_DATE_REG_ADDR,				
 	DS1307_RTC_MONTH_CENTURY_REG_ADDR,		
 	DS1307_RTC_YEAR_REG_ADDR,
	DS1307_RTC_MAX_TIME_REG_NUM,
};

#define DS1307_RTC_HOURS_12_AM_PM		(0x40)
#define DS1307_RTC_HOURS_24				(0x00)

#define DS1307_RTC_CONTROL_REG_ADDR		(0x07)

#define DS1307_RTC_NAME "ds1307"

struct ds1307_rtc_dev {
	struct i2c_client *client;
	struct rtc_device *rtc;
	struct regmap *regmap; 
	int epoch;
};

static int ds1307_rtc_get_time(struct device *dev, struct rtc_time *t)
{
	struct ds1307_rtc_dev *ds1307 = dev_get_drvdata(dev);	
	int ret = 0, regs[DS1307_RTC_MAX_TIME_REG_NUM];

	ret = regmap_bulk_read(ds1307->regmap, DS1307_RTC_SECONDS_REG_ADDR, regs, sizeof(regs));
	if(ret)
		return ret;

	t->tm_sec = bcd2bin(regs[DS1307_RTC_SECONDS_REG_ADDR]);
	t->tm_min = bcd2bin(regs[DS1307_RTC_MINUTES_REG_ADDR]);
	t->tm_hour = bcd2bin(regs[DS1307_RTC_HOURS_REG_ADDR]);
	t->tm_wday = bcd2bin(regs[DS1307_RTC_DAY_REG_ADDR]);

	t->tm_mday = bcd2bin(regs[DS1307_RTC_DATE_REG_ADDR]);
	t->tm_mon = bcd2bin(regs[DS1307_RTC_MONTH_CENTURY_REG_ADDR]) - 1;
	t->tm_year = bcd2bin(regs[DS1307_RTC_YEAR_REG_ADDR]) + ds1307->epoch;

	return 0;
}

static int ds1307_rtc_set_time(struct device *dev, struct rtc_time *t)
{
	pr_info("%s", __func__);

	return 0;
}

static const struct rtc_class_ops ds1307_rtc_ops = {
	.read_time = ds1307_rtc_get_time,
	.set_time = ds1307_rtc_set_time,
};

static int ds1307_rtc_parse_dt(void)
{
	return 0;
}

static const struct regmap_config ds1307_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int ds1307_rtc_probe(struct i2c_client *client,
                         const struct i2c_device_id *id)
{
    int err = 0;
	struct ds1307_rtc_dev *ds1307;

	if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
	{
		pr_err("No support i2c smbus");	
		return -EIO;
	}

	ds1307 = devm_kzalloc(&client->dev, sizeof(*ds1307), GFP_KERNEL);
	if(IS_ERR(ds1307))
		return PTR_ERR(ds1307);
	
	ds1307->client = client;
	ds1307->regmap = devm_regmap_init_i2c(client, &ds1307_regmap_config);
	if (IS_ERR(ds1307->regmap)) 
		return PTR_ERR(ds1307->regmap);
	
	ds1307->epoch = 100;
	dev_set_drvdata(&client->dev, ds1307);

	ds1307->rtc = devm_rtc_device_register(&client->dev, client->name, &ds1307_rtc_ops, THIS_MODULE);
	if(IS_ERR(ds1307->rtc))
		return PTR_ERR(ds1307->rtc);
	
	pr_info("ds1307 Probe Done!!!\n");   	
	return 0;
}

static int ds1307_rtc_remove(struct i2c_client *client)
{   
    pr_info("ds1307 Removed!!!\n");
    return 0;
}

static const struct of_device_id ds1307_rtc_dts[] = {
        { .compatible = "maxim,ds1307" },
        {}
};
MODULE_DEVICE_TABLE(of, ds1307_rtc_dts);

static struct i2c_driver ds1307_rtc_driver = {
        .driver = {
            .name   = DS1307_RTC_NAME,
            .owner  = THIS_MODULE,
			.of_match_table = ds1307_rtc_dts,
        },
        .probe          = ds1307_rtc_probe,
        .remove         = ds1307_rtc_remove,
};

static int __init ds1307_rtc_init(void)
{
    return i2c_add_driver(&ds1307_rtc_driver);
}

static void __exit ds1307_rtc_exit(void)
{
    i2c_del_driver(&ds1307_rtc_driver);
}

module_init(ds1307_rtc_init);
module_exit(ds1307_rtc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yohan Yoon <dbsdy1235@gmail.com>");
MODULE_DESCRIPTION("DS1307 Device Driver");

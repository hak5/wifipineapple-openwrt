#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/ath9k_platform.h>
#include <asm/mach-ath79/ar71xx_regs.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/platform/ar934x_nfc.h>

#include "common.h"
#include "dev-ap9x-pci.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-nfc.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"
#include "dev-m25p80.h"

#define TETRA_GPIO_LED_AR9580   2
#define TETRA_GPIO_LED_STATUS	14
#define TETRA_GPIO_LED_WAN		19

#define TETRA_GPIO_BTN_RESET	17

#define TETRA_KEYS_POLL_INTERVAL		20	/* msecs */
#define TETRA_KEYS_DEBOUNCE_INTERVAL	(3 * TETRA_KEYS_POLL_INTERVAL)


static struct gpio_led tetra_leds_gpio[] __initdata = {
	{
		.name		= "tetra:red",
		.gpio		= TETRA_GPIO_LED_AR9580,
		.active_low	= 1,
	},
	{
		.name		= "tetra:blue",
		.gpio		= TETRA_GPIO_LED_STATUS,
		.active_low	= 1,
	},
	{
		.name		= "tetra:yellow",
		.gpio		= TETRA_GPIO_LED_WAN,
		.active_low	= 1,
	},
};

static struct gpio_keys_button tetra_gpio_keys[] __initdata = {
	{
		.desc		= "Reset Button",
		.type		= EV_KEY,
		.code		= KEY_RESTART,
		.debounce_interval = TETRA_KEYS_DEBOUNCE_INTERVAL,
		.gpio		= TETRA_GPIO_BTN_RESET,
		.active_low     = 1,
	},
};


static void __init tetra_setup(void)
{
	unsigned int old_func, new_func;
    // request resource
	void __iomem *TETRA_GPIO_FUNC = ioremap_nocache(AR71XX_GPIO_BASE + 0x6c, 0x04);
    // Disable JTAG function
	old_func = __raw_readl(TETRA_GPIO_FUNC);
	new_func = old_func | (1 << 1);
	__raw_writel(new_func, TETRA_GPIO_FUNC);
    // free resource
	iounmap(TETRA_GPIO_FUNC);

	ath79_register_leds_gpio(-1, ARRAY_SIZE(tetra_leds_gpio),
		tetra_leds_gpio);
	ath79_register_gpio_keys_polled(-1, TETRA_KEYS_POLL_INTERVAL,
		ARRAY_SIZE(tetra_gpio_keys),
		tetra_gpio_keys);

	ath79_register_m25p80(NULL);

	ath79_setup_ar934x_eth_cfg(AR934X_ETH_CFG_SW_ONLY_MODE);
	ath79_register_mdio(1, 0x0);

	/* GMAC1 is connected to the internal switch */
	ath79_eth1_data.phy_if_mode = PHY_INTERFACE_MODE_GMII;
	ath79_register_eth(1);

	/* GMAC0 is connected to the PHY4 of the internal switch */
	ath79_switch_data.phy4_mii_en = 1;
	ath79_switch_data.phy_poll_mask = BIT(4);
	ath79_eth0_data.phy_if_mode = PHY_INTERFACE_MODE_MII;
	ath79_eth0_data.phy_mask = BIT(4);
	ath79_eth0_data.mii_bus_dev = &ath79_mdio1_device.dev;
	ath79_register_eth(0);

	ath79_nfc_set_ecc_mode(AR934X_NFC_ECC_HW);
	ath79_register_nfc();
	ath79_register_usb();

	ath79_register_wmac_simple();
	ap91_pci_init_simple();
}

MIPS_MACHINE(ATH79_MACH_PINEAPPLE_TETRA, "PINEAPPLE-TETRA",
	"WiFi Pineapple TETRA", tetra_setup);
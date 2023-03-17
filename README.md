# Linux Backlight

According to Arch Linux wiki,

> There are many ways to control brightness of
> a monitor, laptop or integrated panel (such as the iMac).
> The control method can be divided into these categories:
>
> - brightness is controlled by vendor-specified hotkey
> and there is no interface for the OS to adjust the brightness.
> - brightness is controlled by either the ACPI, graphic or platform driver.
> In this case, backlight control is exposed to the user
> through `/sys/class/backlight`
> which can be used by user-space backlight utilities.
> - brightness is controlled by
> writing into a graphic card register through setpci.

This program uses `/sys/class/backlight` to read and controlled brightness.
`setuid` bit and function `setuid()` are used,
so that root permission is not required to run this program.

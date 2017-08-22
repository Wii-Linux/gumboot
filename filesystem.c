
static u8 open_part_no = 0xFF;
static FATFS fatfs;

int config_open_fs(u8 part_no) {
	if (part_no == open_part_no)
		return FR_OK;

	FRESULT res = f_mount(part_no, &fatfs);
	if (res != FR_OK) {
		return (int)res;
	}
	open_part_no = part_no;
	return FR_OK;
}

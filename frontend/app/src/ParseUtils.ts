class ParseUtils {
    getDeviceNameFromDescriptor(device: string): string {
        return device.substring(9, device.indexOf('| 0x')-1);
    }
    
    getDeviceIdFromDescriptor(device: string): string {
        return device.substring(device.indexOf('| 0x')+2);
    }
    //September 21 14:05:09
    formatDate(date: Date): string {
        const months = [
            "January", "February", "March", "April", "May", "June",
            "July", "August", "September", "October", "November", "December"
        ];

        const month = months[date.getMonth()];
        const day = String(date.getDate()).padStart(2, "0");
        const hours = String(date.getHours()).padStart(2, "0");
        const minutes = String(date.getMinutes()).padStart(2, "0");
        const seconds = String(date.getSeconds()).padStart(2, "0");

        return `${month} ${day} ${hours}:${minutes}:${seconds}`;
    }
}

export default new ParseUtils() as ParseUtils;
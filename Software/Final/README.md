# Final Software
Finalized software for MInG. Built with ESP-IDF.

Default access point username: MInG 

Default access point password: mypassword 

![image](https://user-images.githubusercontent.com/72418944/224905057-38abe480-b193-4b59-a3d4-ed91a5137d95.png)


## Features

### Password Management System
- Stores all credentials in flash memory
- Search bar to look up credentials
- Add new credentials through form
- Delete button to remove credentials
- 192.168.4.1/pm

<kbd><img src="https://user-images.githubusercontent.com/72418944/224902820-93ac0810-52f3-4b2a-9637-1af7684908b8.png" /></kbd>

### Config
- Change SSID's name and password
  - Stored in NVS
  - Please use reasonable SSID name/password, otherwise device may not work
- Send any HID text wirelessly
- 192.168.4.1

<kbd><img src="https://user-images.githubusercontent.com/72418944/224903731-ee95bd27-bb79-4396-b782-8056dabc45a0.png" /></kbd>


### Keyboard
- Type any character into target device. Worked on all tested USB-A and USB-C devices that supports keyboards
- Only characters and special characters have been implemented (no delete, tab, F-keys, etc.)
- Only typing on a physical keyboard has been implemented. Tapping on the virtual keyboard will not work. 
- 192.168.4.1/keyboard

<kbd><img src="https://user-images.githubusercontent.com/72418944/224904186-596eafa0-7a3d-4729-99ec-dcc260f0cc6e.png" /></kbd>

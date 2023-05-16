docker run -itd --rm --privileged \
    --device=/dev/disk6:/dev/disk \
    --device=/dev/cu.usbserial-1234_tul0:/dev/cu.usbserial-1234_tul0 \
    --device=/dev/cu.usbserial-1234_tul1:/dev/cu.usbserial-1234_tul1 \
    --device=/dev/tty.usbserial-1234_tul0:/dev/tty.usbserial-1234_tul0 \
    --device=/dev/tty.usbserial-1234_tul1:/dev/tty.usbserial-1234_tul1 \
    --name OSComp whatcanyousee/oslab

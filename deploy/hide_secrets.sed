s/\(_ENDPOINT\\\{0,1\}=\\\{0,1\}"\)[^"]*/\1***/g
/-Xcc -DPROD_AGW_PUBLIC_KEY/,/-Xcc/ { /-Xcc/!d; }
/-Xcc -DPROD_AGW_PUBLIC_KEY/d
/-Xcc -DDEV_AGW_PUBLIC_KEY/,/-Xcc/ { /-Xcc/!d; }
/-Xcc -DDEV_AGW_PUBLIC_KEY/d
/-DPROD_AGW_PUBLIC_KEY/,/-D/ { /-D/!d; }
/-DPROD_AGW_PUBLIC_KEY/d
/-DDEV_AGW_PUBLIC_KEY/,/-D/ { /-D/!d; }
/-DDEV_AGW_PUBLIC_KEY/d

#!/bin/bash

# Создать новую БД с пост-квантовой криптографией
echo "Добавление сервера в БД (VLESS + Post-Quantum)..."

curl -X POST http://31.135.65.188:8080/admin/add-server \
  -H "Content-Type: application/json" \
  -d '{ 
    "country": "Germany", 
    "city": "Frankfurt", 
    "flag": "🇩🇪", 
    "is_premium": true, 
    "type": "xray", 
    "server_host": "138.124.101.69", 
    "xray_inbound_id": 1, 
    "xray_panel_url": "http://138.124.101.69:2096", 
    "xray_username": "admin", 
    "xray_password": "admin", 
    "xray_settings": "{\"port\": 2083, \"flow\": \"xtls-rprx-vision\", \"security\": \"reality\", \"sni\": \"google.com\", \"fingerprint\": \"chrome\", \"public_key\": \"9fSZdYcfxdM99chqGtMrMcEMBocnzivrI74t4Obqqk4\", \"short_id\": \"f31456\", \"spider_x\": \"/\", \"encryption\": \"mlkem768x25519plus.native.0rtt.LUSJiVRfjUFXHHSYhjPuC83PcBp_pb44LTDEOQbcwjo\", \"pqv\": \"H99I0LNs_RgdeAhPLjHW4TuPYV8VAhHx9GcPEMKrtLI_L0Pkf0lZJY9jgVS2qexyrvA1laRScbArf95u_EiSR8mbLJZiHBmLONZ9N_maK_zqOrL09uL7z3F5vC3sAZUQOqkVXEua_IJO0WszH0LW0uh6eQNy8F5HJY4_jpMwZ9bbgdvi_gNptt0yeT57Qbj3NibB3yGJXUDW2vqPx7YDj6-Ggq94ihl3O99dHeKAdvxspMffKXbGprNgqRMyFkaOTSjHwUkRs-c0okVJkzJA3uCy-cZ8EaS0rNqgltmH-PWpZiiqxW5cveYwQjh-NeThEGm3yJ-WweTS8RxG94ufo6OugSLKhDTjkAZDwbpqaryHEYteaNj6BcZWZAzY2olBaxJDEt4T6ZL0M3r1Il4j0_odWDddirzaqhLERoxmMAurtEXlwe-xtPp0eE2goTA002JoLMPqaKp2NppsS-28pz8PXadXtTmq457g_4wwv6JhrajZ2vvNEjNdGTfBwPdd-mc88t2b3HpZJ08b2xUcv7tAuoqsEZgE9-AW_bDg3-rzbtc4VLPlHsrdzoFZo5Fy9VN65MQqbyTarVtz3qNt8SIksRVaswDVeFbUorSyaKslzC7sF_9TTHXlU9OAO259FkoibyC1PYoo7BceC-RNJw9i2s6ppQExLoXa1WoevHYq3EJHGTwOVdYhJ00b-fJCfHpsOP-ul-lQqhvXX627T6FLg13dLUPTyvXcQJiECRZFtFWpbilxr_9dFaMpmjSlED32PLhA4FxI_5uyvnyoaEJrfCgBcoKkOGBVz7e-jwzsBP3m3wrCCgzoICCRBlN35b-ZcmtdHRBr831X6RuiVPvLVVxGB7PqX1Mkj0h8nwc00n3eepL4Xrtq5ViaQKARGFz8WUKMyla0mnsdkwZTFFe4VOnNvK7-CJWDEbTggk_vdH-XKylQr9ROKAACyvlk46hFltRosxjYdlYuiS6iKGGF9wxv4KVHMU79dv-cCd-pxJEgpq2QaCGm6EKnbZXD0WMkeAd_Ao31MYwIM3UkjTidp7u3cYXvBy5Iwoz9i3ZRAfIfT2kKl_VSdHBQY4G2UeK_SjQjY86QLOqJRLT1z8us16fNsZuAteDcVyX0_HPdxuJFnsC_RSYJC2PBPd7khHyXe7wSI-vYwjQ0o3rOOhyw0Y0lvjtvhn1tUQLTfhVEpVUfkBsMEa55nYIRhNQ5c0xREqPyoPE2ItpCm2-ArBzfHCEntd3boNI6DzdCrr8u02E1jXOxyDEWFk8IBciG296tqy1vbmuaeqIhzUwY_cIC7xf1oKzY9a2jos0md49ZW4aEy0NUEc8hc7-Exm8xWaHSzNyS1cB9QyfHPbxgc_16SqG-dmmewsvUtOBXcRymu_wZN9vzt6wLqjtXGbWOlZbsesLwjiTIgeO1WE9-9bfyLzesllWN_5dg5jDFTweZkCV-wCjhh9cKAEsF5XVZ0f4tD3llQTXLvwgKFATL_03G_E4pDGZMZ0Mz9nbsPWWX_Y6v14cu7JGEl1cfz-kMoM8MRZbwSgxHNI2K_iB8GwoUTJuXo4YZ6jYDQ-sU60649_V_LE-ZTdgYamfAzoOMu_W88mlGhzmiMvPXDFJtN_Uu4Tiyl5owTxIePYxEn6DrCVr64-7P0VRmr44_1_6DgJuhvnAoM0VSy7VE7TyadpZk6RLRq8Zt9GIyjeKJpLZVDZNvWohAh3FBuMhWlI0bthDJvDo2w0Ro0YviXRCYdQ_WcyIRuZ4mJx5Mc0d69nmE4_OoNH655gM6KjVCLk9hZJH7T2iQ2jtjS4xaKgjUxPuFzRujfwWLLqqaii02yfnCH9aISaN25BsDpHKgGOX0u0XlJnoQfsqYtxZQRLx0VdB-0bms1ExZyB8b5jZrrckbmjE59T6uAEvWQLXRQoc93tGT7IubvxLYc2C4ENc7BTZagKHVPLGvZY8VDTrBFrh1htZuIoX9eEqHhWa1De5mVayHcXTZ4G9HQjLZ3bY00HmDWKHKnP6RBGMO6H-_O4jegjhZ3EfzN91nDbeml4K3x-yuwGXrkdZT-oE6Bk22rxUZIVSPAl10irtHcJwlCLUJVRlwsVVjdy0vbU0mqww9h5O2lBVy6OxCU1C0z0wwJkCL_UpsoVgpw8UnVtbnvYprZC0V2tFoKZe9ZulpfOnfisuOhSR3kNzn5XQ0PQa1K8t1GqlHvKCZq0BKtx1JfoJfz8YNnq2hqD-b5PvKLKuxhUM1nz2x87CgoFvigE1oA_8kxZ3LmO9fg__NLwwzvhLGInISQJno8HEwPC7uK9Aiypb6bzQbt_-8XEeNFR9TxJsQ9DyjtevUgc-Th61Tbj0gJOHEv2XDs2WmNU2HPpSeVi6IQ1IbolKFp-5OmuN_lC3fwSSAyS0B0gnmUfuZao55MqRR8j0f1oELY1O58MKl6lncDLl9AhenwXIOGajb2usiUQ8D5ulPwKODUnTO9CeL2ZMVsSGRlVyAq_qiPLevko19KH4d__yFtgjc7VuQVB0d5abx57-7j7KKRidjTK9I63UWdnhKAJj-zaipz6YE_r56DmJnuu12jFRCwY_LdNHjahcYy_U0z4-AeSSQg5c\"}" \
  }'

echo
echo "Готово!"

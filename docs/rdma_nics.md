# RDMA capable NICs

|     | NIC Series                                |  RoCE v1 | RoCE v2 | iWARP | Details                                       | Tested |
| --- | ----------------------------------------- | -------- | ------- | ----- | --------------------------------------------- | ------ |
| 1   | Generic Non-RDMA Ethernet Cards           | 🔴       | 🟡       | 🟡    | Realtek "network cards" etc.                  |        |
| 2   | HP NC523SFP (QLogic cLOM8214)             | 🔴       | 🟡       | 🟡    | Ethernet 10GbE                                | ✔️     |
| 3   | Mellanox ConnectX-3 EN                    | 🟢       | 🟡       | 🟡    | Ethernet 10GbE                                | ✔️     |
| 4   | Broadcom/Emulex OneConnect OCe14000       | 🟢       | 🟡       | 🟡    | Ethernet 10/40GbE                             |        |
| 5   | Mellanox ConnectX-3 Pro EN                | 🟢       | 🟢       | 🟡    | Ethernet 10/40/56GbE                          |        |
| 6   | Mellanox ConnectX-4 Lx EN                 | 🟢       | 🟢       | 🟡    | Ethernet 10/25/40/50GbE                       |        |
| 7   | Intel X722                                | 🔴       | 🟡       | 🟢    | Ethernet 10GbE                                |        |
| 8   | Marvell/QLogic FastLinQ 41000             | 🟢       | 🟢       | 🟢    | Ethernet 10/25GbE                             |        |
| 9   | Marvell/QLogic FastLinQ 45000             | 🟢       | 🟢       | 🟢    | Ethernet 10/25/40/100GbE                      |        |

🔴 Unsupported  
🟡 Software only  
🟢 Hardware support

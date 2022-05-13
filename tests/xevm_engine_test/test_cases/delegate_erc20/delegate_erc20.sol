// SPDX-License-Identifier: MIT
pragma solidity >=0.5.12;

contract TopErc20Wrapper {
    address constant TopErc20 = 0xff00000000000000000000000000000000000001;

    string public name;
    string public symbol;
    uint8 public decimals;
    bytes1 public uuid;

    constructor(string memory name_, string memory symbol_, uint8 decimals_, bytes1 uuid_) {
        name = name_;
        symbol = symbol_;
        decimals = decimals_;
        uuid = uuid_;
    }

    fallback() external {
        bytes memory call_data = abi.encodePacked(uuid, msg.data);
        (bool success, bytes memory result) = TopErc20.delegatecall(call_data);

        require(success, string(result));

        assembly {
            return(add(result, 0x20), mload(result))
        }
    }
}

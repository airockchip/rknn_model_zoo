
chip_name=0
freq_set_status=0

usage()
{
    echo "USAGE: ./scaling_frequency.sh -c {chip_name} [-h]"
    echo "  -c:  chip_name, such as rv1126 / rk3588"
    echo "  -h:  Help"
}

# print_and_compare_result want result
print_and_compare_result()
{
    # echo "compare result"
    echo "    try set "$1
    echo "    query   "$2
    if [ $1 == $2 ];then
        echo "    Seting Success"
    else
        echo "    Seting Failed"
        freq_set_status=-1
    fi
}

# print_not_support_adjust {device} {freq want} {freq want}
print_not_support_adjust()
{
    echo "Firmware seems not support seting $1 frequency"
    echo "    wanted "$2
    echo "    check  "$3
}


# MAIN FUNCTION HERE #
vaild=0

if [ $# == 0 ]; then
    usage
    exit 0
fi

while getopts "c:dc" arg
do
    case $arg in
        c)
          chip_name=$OPTARG
          ;;
        h)
          usage
          exit 0
          ;;
        ?)
          usage
          exit 1
          ;;
    esac
done


if [ $chip_name == 'rv1126' ]; then
    seting_strategy=1
    CPU_freq=1512000
    NPU_freq=934000000
    DDR_freq=924000000
elif [ $chip_name == 'rv1109' ]; then
    seting_strategy=1
    CPU_freq=1512000
    NPU_freq=594000000
    DDR_freq=924000000
elif [ $chip_name == 'rk1808' ]; then
    seting_strategy=1
    # CPU_freq=1512000
    CPU_freq=1200000
    NPU_freq=792000000
    DDR_freq=924000000
elif [ $chip_name == 'rk3399pro' ]; then
    seting_strategy=2
    CPU_freq=1512000
    NPU_freq=792000000
    DDR_freq=934000000
elif [ $chip_name == 'rk3562' ]; then
    seting_strategy=6
    CPU_freq=2000000
    NPU_freq=1000000000
    DDR_freq=1560000000
elif [ $chip_name == 'rk3566' ]; then
    seting_strategy=3
    CPU_freq=1800000
    NPU_freq=1000000000
    DDR_freq=1056000000
elif [ $chip_name == 'rk3568' ]; then
    seting_strategy=3
    CPU_freq=1800000
    NPU_freq=1000000000
    DDR_freq=1560000000
elif [ $chip_name == 'rk3588' ]; then
    seting_strategy=4
    CPU_freq=2256000
    NPU_freq=1000000000
    DDR_freq=2112000000
elif [ $chip_name == 'rk3576' ]; then
    seting_strategy=7
    CPU_freq=2016000
    NPU_freq=1000000000
    DDR_freq=2112000000
elif [ $chip_name == 'rv1106' ]; then
    seting_strategy=5
    CPU_freq=1608000
    NPU_freq=500000000
    DDR_freq=924000000
elif [ $chip_name == 'rv1103' ]; then
    seting_strategy=5
    CPU_freq=1608000
    NPU_freq=500000000
    DDR_freq=924000000
else 
    echo "$chip_name not recognize"
    exit 1
fi



echo "Try seting frequency for "${chip_name}
echo "    Setting strategy as $seting_strategy"
echo "    NPU seting to "$NPU_freq
echo "    CPU seting to "$CPU_freq
echo "    DDR seting to "$DDR_freq


case $seting_strategy in
    0) 
        echo "seting strategy not implement now"
        ;;
    
    # for rk1808, rv1126, rv1109
    1) 
        echo "CPU: seting frequency"
        echo userspace > /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpu0/cpufreq/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "NPU: seting frequency"
        echo userspace > /sys/class/devfreq/ffbc0000.npu/governor
        echo $NPU_freq > /sys/class/devfreq/ffbc0000.npu/userspace/set_freq
        NPU_cur_freq=$(cat /sys/class/devfreq/ffbc0000.npu/cur_freq)
        print_and_compare_result $NPU_freq $NPU_cur_freq

        echo "DDR: seting frequency"
        if [ $chip_name == 'rk1808' ]; then
            cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep pll_dpll | awk '{split($0,a," "); print a[4]}')
        else
            cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep pll_dpll | awk '{split($0,a," "); print a[5]}')
        fi
        DDR_cur_freq="`expr $cur_freq \* 2`"
        print_not_support_adjust DDR $DDR_freq $DDR_cur_freq
        ;;

    # for rk3399pro
    2)
        echo "seting strategy not implement for rk3399pro now"
        ;;

    # for rk3566, rk3568
    3)
        echo "CPU: seting frequency"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "NPU: seting frequency"
        echo userspace > /sys/class/devfreq/fde40000.npu/governor
        echo $NPU_freq > /sys/class/devfreq/fde40000.npu/userspace/set_freq
        NPU_cur_freq=$(cat /sys/devices/platform/fde40000.npu/devfreq/fde40000.npu/cur_freq)
        print_and_compare_result $NPU_freq $NPU_cur_freq

        echo "DDR: seting frequency"
        echo userspace > /sys/class/devfreq/dmc/governor
        echo $DDR_freq > /sys/class/devfreq/dmc/userspace/set_freq
        DDR_cur_freq=$(cat /sys/class/devfreq/dmc/cur_freq)
        print_and_compare_result $DDR_freq $DDR_cur_freq
        ;;

    # for rk3588
    4)
        echo "CPU: seting frequency"
        echo "  Core0"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
        echo 1800000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result 1800000 $CPU_cur_freq

        echo "  Core4"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy4/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpufreq/policy4/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "  Core6"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy6/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpufreq/policy6/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu6/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "NPU: seting frequency"
        if [ -e  /sys/class/devfreq/fdab0000.npu/governor ];then
            echo userspace > /sys/class/devfreq/fdab0000.npu/governor 
            echo $NPU_freq > /sys/class/devfreq/fdab0000.npu/userspace/set_freq 
            NPU_cur_freq=$(cat /sys/class/devfreq/fdab0000.npu/cur_freq)
        elif [ -e /sys/class/devfreq/devfreq0/governor ];then
            echo performance > /sys/class/devfreq/devfreq0/governor 
            NPU_cur_freq=$(cat /sys/class/devfreq/devfreq0/cur_freq)
        else
            NPU_cur_freq=$(cat /sys/kernel/debug/clk/scmi_clk_npu/clk_rate)
        fi
        print_not_support_adjust NPU $NPU_freq $NPU_cur_freq

        echo "DDR: seting frequency"
        if [ -e /sys/class/devfreq/dmc/governor ];then
            echo userspace > /sys/class/devfreq/dmc/governor
            echo $DDR_freq > /sys/class/devfreq/dmc/userspace/set_freq
            DDR_cur_freq=$(cat /sys/class/devfreq/dmc/cur_freq)
            print_and_compare_result $DDR_freq $DDR_cur_freq
        else
            print_not_support_adjust DDR $DDR_freq
            $DDR_cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep scmi_clk_ddr)
            echo $DDR_cur_freq
        fi

        ;;

    # rv1106, rv1103
    5)
        echo "CPU: seting frequency"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "NPU: seting frequency"
        # echo "  no strategy to seting DDR frequency"
        # echo $NPU_freq > /sys/kernel/debug/clk/aclk_npu_root/clk_rate
        echo $NPU_freq > /sys/kernel/debug/clk/clk_500m_src/clk_rate
        NPU_cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep aclk_npu_root | awk '{split($0,a," "); print a[5]}')
        print_and_compare_result $NPU_freq $NPU_cur_freq

        echo "DDR: seting frequency"
        echo "  no strategy to seting DDR frequency"
        cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep clk_core_ddrc_src | awk '{split($0,a," "); print a[5]}')
        DDR_cur_freq="`expr $cur_freq \* 2`"
        print_and_compare_result $DDR_freq $DDR_cur_freq
        ;;

    # rk3562
    6)
        echo "CPU: seting frequency"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "NPU: seting frequency"
        echo userspace > /sys/devices/platform/ff300000.npu/devfreq/ff300000.npu/governor
        echo $NPU_freq > /sys/devices/platform/ff300000.npu/devfreq/ff300000.npu/userspace/set_freq
        NPU_cur_freq=$(cat /sys/class/devfreq/ff300000.npu/cur_freq)
        print_and_compare_result $NPU_freq $NPU_cur_freq

        echo "DDR: seting frequency"
        if [ -e /sys/class/devfreq/dmc/governor ];then
            echo userspace > /sys/class/devfreq/dmc/governor
            echo $DDR_freq > /sys/class/devfreq/dmc/userspace/set_freq
            DDR_cur_freq=$(cat /sys/class/devfreq/dmc/cur_freq)
        else
            DDR_cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep scmi_clk_ddr | awk '{split($0,a," "); print a[5]}')
        fi
        print_not_support_adjust DDR $DDR_freq $DDR_cur_freq
        ;;

    # rk3576
    7)
        echo "CPU: seting frequency"
        echo "  Core0"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy0/scaling_governor
        echo 1800000 > /sys/devices/system/cpu/cpufreq/policy0/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpufreq/policy0/scaling_cur_freq)
        print_and_compare_result 1800000 $CPU_cur_freq

        echo "  Core4"
        echo userspace > /sys/devices/system/cpu/cpufreq/policy4/scaling_governor
        echo $CPU_freq > /sys/devices/system/cpu/cpufreq/policy4/scaling_setspeed
        CPU_cur_freq=$(cat /sys/devices/system/cpu/cpufreq/policy4/scaling_cur_freq)
        print_and_compare_result $CPU_freq $CPU_cur_freq

        echo "NPU: seting frequency"
        echo userspace > /sys/devices/platform/27700000.npu/devfreq/27700000.npu/governor
        echo $NPU_freq > /sys/class/devfreq/27700000.npu/userspace/set_freq 
        NPU_cur_freq=$(cat /sys/class/devfreq/27700000.npu/cur_freq)
        print_and_compare_result $NPU_freq $NPU_cur_freq

        echo "DDR: seting frequency"
        if [ -e /sys/class/devfreq/dmc/governor ];then
            echo userspace > /sys/class/devfreq/dmc/governor
            echo $DDR_freq > /sys/class/devfreq/dmc/userspace/set_freq
            DDR_cur_freq=$(cat /sys/class/devfreq/dmc/cur_freq)
        else
            DDR_cur_freq=$(cat /sys/kernel/debug/clk/clk_summary | grep scmi_clk_ddr | awk '{split($0,a," "); print a[5]}')
        fi
        print_not_support_adjust DDR $DDR_freq $DDR_cur_freq
        ;;

    *)
        echo "seting strategy not implement now"
        ;;
esac


echo "freq set" > ./freq_set_status
echo "CPU: "$CPU_freq >> ./freq_set_status
echo "NPU: "$NPU_freq >> ./freq_set_status
echo "DDR: "$DDR_freq >> ./freq_set_status
echo "freq query" >> ./freq_set_status
echo "CPU: "$CPU_cur_freq >> ./freq_set_status
echo "NPU: "$NPU_cur_freq >> ./freq_set_status
echo "DDR: "$DDR_cur_freq >> ./freq_set_status
if [ $freq_set_status == 0 ];then
    echo "Seting Success" >> ./freq_set_status
else
    echo "Seting Failed" >> ./freq_set_status
fi

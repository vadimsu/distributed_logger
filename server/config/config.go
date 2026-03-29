package config

type ListenerConfig struct{
        Port string `json:"port"`
        Ip string `json:"ip"`
}

type Certificate struct{
        Server string `json:"server"`
        Key string `json:"key"`
        Root string `json:"root"`
}

type Config struct{
        Listener ListenerConfig `json:"listener"`
        Certificates Certificate `json:"certificate"`
}

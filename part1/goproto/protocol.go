package part1

import (
	"encoding/json"
	//"bytes"
	//"encoding/binary"
	"log"
)

type MyRPC struct {
	FunctionId uint8 // 1 for roundtrip echo
	Flag       uint8 // 0 for MSG, 1 for ACK
	ActualSize uint32
	Data       []byte
}

func (r MyRPC) MarshalBinary() ([]byte, error) {
	return json.Marshal(r)
	//	fmt.Fprint(&b, r.FunctionId, r.Flag, r.ActualSize, r.Data)
	//	return b.Bytes(), nil
}

func (r *MyRPC) UnMarshalBinary(data []byte) error {
	return json.Unmarshal(data, r)
	//	b := bytes.NewBuffer(data)
	//	_, err := fmt.Fscan(b, &r.FunctionId, &r.Flag, &r.ActualSize, &r.Data)
	//	return err
}

func CheckError(err error) {
	if err != nil {
		log.Fatal("CheckError:", err)
	}
}

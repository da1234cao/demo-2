package certificate

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"crypto/x509/pkix"
	"encoding/pem"
	"io/ioutil"
	"math/big"
	"time"
)

func Gencertificate(output string) error {
	// ref: https://foreverz.cn/go-cert

	// 生成私钥
	priv, err := rsa.GenerateKey(rand.Reader, 2048)
	if err != nil {
		return err
	}

	// x509证书内容
	var csr = &x509.Certificate{
		Version:      3,
		SerialNumber: big.NewInt(time.Now().Unix()),
		Subject: pkix.Name{
			Country:            []string{"CN"},
			Province:           []string{"Shanghai"},
			Locality:           []string{"Shanghai"},
			Organization:       []string{"httpsDemo"},
			OrganizationalUnit: []string{"httpsDemo"},
			CommonName:         "da1234cao.top",
		},
		NotBefore:             time.Now(),
		NotAfter:              time.Now().AddDate(1, 0, 0),
		BasicConstraintsValid: true,
		IsCA:                  false,
		KeyUsage:              x509.KeyUsageDigitalSignature | x509.KeyUsageKeyEncipherment,
		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
	}

	// 证书签名
	certDer, err := x509.CreateCertificate(rand.Reader, csr, csr, priv.Public(), priv)
	if err != nil {
		return err
	}

	// 二进制证书解析
	interCert, err := x509.ParseCertificate(certDer)
	if err != nil {
		return err
	}

	// 证书写入文件
	pemData := pem.EncodeToMemory(&pem.Block{
		Type:  "CERTIFICATE",
		Bytes: interCert.Raw,
	})
	if err = ioutil.WriteFile(output+"cert.pem", pemData, 0644); err != nil {
		panic(err)
	}

	// 私钥写入文件
	keyData := pem.EncodeToMemory(&pem.Block{
		Type:  "EC PRIVATE KEY",
		Bytes: x509.MarshalPKCS1PrivateKey(priv),
	})

	if err = ioutil.WriteFile(output+"key.pem", keyData, 0644); err != nil {
		return err
	}

	return nil
}
